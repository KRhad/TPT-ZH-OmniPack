#include "Rusanov1D.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <utility>
#include <vector>

namespace omni::atmospherebench
{

namespace
{
	constexpr double Gamma = 5.0 / 3.0;
	constexpr double SpecificGasConstant = 1.0;
	constexpr double CellLength = 1.0;
	constexpr std::size_t UniformCells = 64;
	constexpr std::size_t UniformSteps = 16;
	constexpr double UniformTimeStep = 0.05;
	constexpr std::size_t PressurePulseCells = 128;
	constexpr std::size_t PressurePulseSteps = 64;
	constexpr double PressurePulseTimeStep = 0.02;
	constexpr double PressurePulseAmplitude = 0.1;
	constexpr double PressurePulseWidthCells = 8.0;

	ConservativeState Add(const ConservativeState &left, const ConservativeState &right)
	{
		return {
			left.density + right.density,
			left.momentumX + right.momentumX,
			left.momentumY + right.momentumY,
			left.totalEnergyDensity + right.totalEnergyDensity,
		};
	}

	ConservativeState Subtract(const ConservativeState &left, const ConservativeState &right)
	{
		return {
			left.density - right.density,
			left.momentumX - right.momentumX,
			left.momentumY - right.momentumY,
			left.totalEnergyDensity - right.totalEnergyDensity,
		};
	}

	ConservativeState Scale(double factor, const ConservativeState &state)
	{
		return {
			factor * state.density,
			factor * state.momentumX,
			factor * state.momentumY,
			factor * state.totalEnergyDensity,
		};
	}

	ConservativeState TotalState(const std::vector<ConservativeState> &cells)
	{
		ConservativeState total;
		for (const auto &cell : cells)
			total = Add(total, cell);
		return total;
	}

	ConservativeState FluxX(const ConservativeState &state, const PrimitiveState &primitive)
	{
		return {
			state.momentumX,
			state.momentumX * primitive.velocityX + primitive.pressure,
			state.momentumY * primitive.velocityX,
			primitive.velocityX * (state.totalEnergyDensity + primitive.pressure),
		};
	}

	ConservativeState RusanovFluxX(
		const ConservativeState &left,
		const ConservativeState &right,
		const IdealGasEOS &eos)
	{
		const auto leftPrimitive = eos.ToPrimitive(left);
		const auto rightPrimitive = eos.ToPrimitive(right);
		if (!leftPrimitive.valid || !rightPrimitive.valid)
			return {};
		const auto maximumWaveSpeed = std::max(
			std::abs(leftPrimitive.velocityX) + leftPrimitive.soundSpeed,
			std::abs(rightPrimitive.velocityX) + rightPrimitive.soundSpeed
		);
		return Subtract(
			Scale(0.5, Add(FluxX(left, leftPrimitive), FluxX(right, rightPrimitive))),
			Scale(0.5 * maximumWaveSpeed, Subtract(right, left))
		);
	}

	double MaximumPressure(const std::vector<ConservativeState> &cells, const IdealGasEOS &eos)
	{
		double maximum = 0.0;
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid)
				return std::numeric_limits<double>::quiet_NaN();
			maximum = std::max(maximum, primitive.pressure);
		}
		return maximum;
	}

	double StateChangeL1(
		const std::vector<ConservativeState> &initial,
		const std::vector<ConservativeState> &final)
	{
		if (initial.size() != final.size())
			return std::numeric_limits<double>::quiet_NaN();
		double change = 0.0;
		for (std::size_t cell = 0; cell < initial.size(); ++cell)
		{
			change += std::abs(final[cell].density - initial[cell].density);
			change += std::abs(final[cell].momentumX - initial[cell].momentumX);
			change += std::abs(final[cell].momentumY - initial[cell].momentumY);
			change += std::abs(final[cell].totalEnergyDensity - initial[cell].totalEnergyDensity);
		}
		return change;
	}

	RusanovProbeSummary RunPeriodicProbe(
		BenchmarkCase benchmarkCase,
		std::vector<ConservativeState> initialCells,
		const IdealGasEOS &eos,
		bool requireEvolution,
		bool requirePeakReduction,
		double conservationTolerance)
	{
		RusanovProbeSummary summary{benchmarkCase};
		if (!summary.benchmarkCase.IsValid() || !eos.IsValid()
			|| initialCells.size() != summary.benchmarkCase.grid.CellCount())
			return summary;

		std::vector<ConservativeState> cells = initialCells;
		std::vector<ConservativeState> fluxes(cells.size());
		std::vector<ConservativeState> next(cells.size());
		const double lambda = summary.benchmarkCase.timeStep / summary.benchmarkCase.grid.cellLength;
		summary.ledger.Begin(TotalState(cells));
		summary.initialMaximumPressure = MaximumPressure(cells, eos);
		if (!std::isfinite(summary.initialMaximumPressure))
			return summary;

		for (std::size_t step = 0; step < summary.benchmarkCase.stepCount; ++step)
		{
			double stepCfl = 0.0;
			for (const auto &cell : cells)
			{
				const auto primitive = eos.ToPrimitive(cell);
				if (!primitive.valid)
					return summary;
				stepCfl = std::max(stepCfl,
					lambda * (std::abs(primitive.velocityX) + primitive.soundSpeed));
			}
			summary.maximumCfl = std::max(summary.maximumCfl, stepCfl);
			if (!std::isfinite(stepCfl) || stepCfl <= 0.0 || stepCfl > 1.0)
				return summary;

			for (std::size_t cell = 0; cell < cells.size(); ++cell)
				fluxes[cell] = RusanovFluxX(cells[cell], cells[(cell + 1) % cells.size()], eos);
			for (std::size_t cell = 0; cell < cells.size(); ++cell)
			{
				const auto leftFace = fluxes[(cell + cells.size() - 1) % cells.size()];
				next[cell] = Subtract(cells[cell], Scale(lambda, Subtract(fluxes[cell], leftFace)));
				if (!eos.ToPrimitive(next[cell]).valid)
					return summary;
			}
			cells.swap(next);
		}

		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		summary.minimumEnergyDensity = std::numeric_limits<double>::infinity();
		summary.finalMaximumPressure = 0.0;
		summary.positivityPreserved = true;
		for (const auto &cell : cells)
		{
			const auto primitive = eos.ToPrimitive(cell);
			if (!primitive.valid)
			{
				summary.positivityPreserved = false;
				break;
			}
			summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
			summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
			summary.minimumEnergyDensity = std::min(summary.minimumEnergyDensity, cell.totalEnergyDensity);
			summary.finalMaximumPressure = std::max(summary.finalMaximumPressure, primitive.pressure);
		}
		summary.stateChangeL1 = StateChangeL1(initialCells, cells);
		summary.stateEvolved = std::isfinite(summary.stateChangeL1) && summary.stateChangeL1 > 1e-12;
		summary.pressurePeakReduced = summary.finalMaximumPressure < summary.initialMaximumPressure;
		summary.ledger.End(TotalState(cells));
		summary.passed = summary.positivityPreserved
		&& summary.ledger.Closes(conservationTolerance)
		&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
		&& (!requireEvolution || summary.stateEvolved)
		&& (!requirePeakReduction || summary.pressurePeakReduced);
		return summary;
	}
}

RusanovProbeSummary RunRusanovUniform()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto initial = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	return RunPeriodicProbe(
		{"rusanov_uniform_1d", {UniformCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, UniformTimeStep, UniformSteps},
		std::vector<ConservativeState>(UniformCells, initial), eos, false, false, 1e-12);
}

RusanovProbeSummary RunRusanovPressurePulse()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(PressurePulseCells);
	const double center = static_cast<double>(PressurePulseCells / 2);
	for (std::size_t cell = 0; cell < PressurePulseCells; ++cell)
	{
		const double distance = static_cast<double>(cell) - center;
		const double pulse = PressurePulseAmplitude * std::exp(
			-(distance * distance) / (2.0 * PressurePulseWidthCells * PressurePulseWidthCells));
		initial[cell] = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0 + pulse);
	}
	return RunPeriodicProbe(
		{"rusanov_pressure_pulse_1d",
			{PressurePulseCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, PressurePulseTimeStep, PressurePulseSteps},
		std::move(initial), eos, true, true, 1e-10);
}

bool WriteRusanovUniformProbe(std::ostream &output)
{
	const auto summary = RunRusanovUniform();
	const auto &initial = summary.ledger.initial;
	const auto &final = summary.ledger.final;
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=fvm_rusanov\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
	output << "numerical_correction_count=" << summary.corrections.eventCount << '\n';
	output << "correction_mass_added=" << summary.corrections.massAdded << '\n';
	output << "correction_mass_removed=" << summary.corrections.massRemoved << '\n';
	output << "correction_momentum_x_added=" << summary.corrections.momentumXAdded << '\n';
	output << "correction_momentum_y_added=" << summary.corrections.momentumYAdded << '\n';
	output << "correction_energy_added=" << summary.corrections.energyAdded << '\n';
	output << "correction_energy_removed=" << summary.corrections.energyRemoved << '\n';
	output << "density_floor_hits=" << summary.corrections.densityFloorHits << '\n';
	output << "pressure_floor_hits=" << summary.corrections.pressureFloorHits << '\n';
	output << "correction_event_count=" << summary.corrections.eventCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

bool WriteRusanovPressurePulseProbe(std::ostream &output)
{
	const auto summary = RunRusanovPressurePulse();
	const auto &initial = summary.ledger.initial;
	const auto &final = summary.ledger.final;
	output << "schema_version=1\n";
	output << "case=" << summary.benchmarkCase.id << '\n';
	output << "candidate=fvm_rusanov\n";
	output << "candidate_solver_implemented=true\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "result_status=candidate_result_not_selection\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
	output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
	output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
	output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
	output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
	output << "maximum_cfl=" << summary.maximumCfl << '\n';
	output << "initial_mass=" << initial.density << '\n';
	output << "final_mass=" << final.density << '\n';
	output << "mass_drift=" << (final.density - initial.density) << '\n';
	output << "momentum_drift=" << std::hypot(
		final.momentumX - initial.momentumX,
		final.momentumY - initial.momentumY
	) << '\n';
	output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
	output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
	output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
	output << "minimum_density=" << summary.minimumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
	output << "initial_maximum_pressure=" << summary.initialMaximumPressure << '\n';
	output << "final_maximum_pressure=" << summary.finalMaximumPressure << '\n';
	output << "pressure_peak_reduced=" << (summary.pressurePeakReduced ? "true" : "false") << '\n';
	output << "state_change_l1=" << summary.stateChangeL1 << '\n';
	output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
	output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
	output << "numerical_correction_count=" << summary.corrections.eventCount << '\n';
	output << "correction_mass_added=" << summary.corrections.massAdded << '\n';
	output << "correction_mass_removed=" << summary.corrections.massRemoved << '\n';
	output << "correction_momentum_x_added=" << summary.corrections.momentumXAdded << '\n';
	output << "correction_momentum_y_added=" << summary.corrections.momentumYAdded << '\n';
	output << "correction_energy_added=" << summary.corrections.energyAdded << '\n';
	output << "correction_energy_removed=" << summary.corrections.energyRemoved << '\n';
	output << "density_floor_hits=" << summary.corrections.densityFloorHits << '\n';
	output << "pressure_floor_hits=" << summary.corrections.pressureFloorHits << '\n';
	output << "correction_event_count=" << summary.corrections.eventCount << '\n';
	output << "state_bytes_per_cell=" << sizeof(ConservativeState) << '\n';
	output << "state_and_flux_scratch_bytes_per_cell=" << (3 * sizeof(ConservativeState)) << '\n';
	output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
