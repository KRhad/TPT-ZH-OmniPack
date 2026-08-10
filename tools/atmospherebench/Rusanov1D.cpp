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
	constexpr std::size_t DensityAdvectionCells = 128;
	constexpr std::size_t DensityAdvectionSteps = 20;
	constexpr double DensityAdvectionTimeStep = 0.1;
	constexpr double DensityAdvectionVelocity = 0.5;
	constexpr double DensityAdvectionAmplitude = 0.2;
	constexpr double Pi = 3.141592653589793238462643383279502884;
	constexpr std::size_t ContactCells = 128;
	constexpr std::size_t ContactSteps = 20;
	constexpr double ContactTimeStep = 0.1;
	constexpr double ContactVelocity = 0.5;
	constexpr double ContactLowDensity = 0.8;
	constexpr double ContactHighDensity = 1.2;

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

	double DensityTotalVariation(const std::vector<ConservativeState> &cells)
	{
		if (cells.empty())
			return 0.0;
		double variation = 0.0;
		for (std::size_t cell = 0; cell < cells.size(); ++cell)
			variation += std::abs(cells[(cell + 1) % cells.size()].density - cells[cell].density);
		return variation;
	}

	RusanovProbeSummary RunPeriodicProbe(
		BenchmarkCase benchmarkCase,
		std::vector<ConservativeState> initialCells,
		const IdealGasEOS &eos,
		bool requireEvolution,
		bool requirePeakReduction,
		double conservationTolerance,
		std::vector<ConservativeState> *finalCellsOutput)
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
		summary.maximumDensity = 0.0;
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
			summary.maximumDensity = std::max(summary.maximumDensity, primitive.density);
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
		if (finalCellsOutput)
			*finalCellsOutput = cells;
		return summary;
	}

	void EvaluateAdvectedDensity(
		RusanovProbeSummary &summary,
		const std::vector<ConservativeState> &initial,
		const std::vector<ConservativeState> &final,
		const IdealGasEOS &eos,
		double referenceVelocity,
		double densityL1Tolerance,
		double densityLinfTolerance,
		double minimumExpectedDensity,
		double maximumExpectedDensity)
	{
		summary.referenceVelocity = referenceVelocity;
		const double shiftReal = referenceVelocity * summary.benchmarkCase.timeStep
			* static_cast<double>(summary.benchmarkCase.stepCount)
			/ summary.benchmarkCase.grid.cellLength;
		const auto shiftRounded = static_cast<long long>(std::llround(shiftReal));
		const bool integralShift = std::abs(shiftReal - static_cast<double>(shiftRounded)) <= 1e-12
			&& shiftRounded >= 0
			&& static_cast<unsigned long long>(shiftRounded)
				< static_cast<unsigned long long>(initial.size());
		if (!integralShift || final.size() != initial.size() || initial.empty())
			return;

		summary.referenceShiftCells = static_cast<std::size_t>(shiftRounded);
		double densityL1 = 0.0;
		double densityLinf = 0.0;
		double pressureLinf = 0.0;
		for (std::size_t cell = 0; cell < final.size(); ++cell)
		{
			const auto referenceCell = initial[
				(cell + initial.size() - summary.referenceShiftCells) % initial.size()];
			const double densityError = std::abs(final[cell].density - referenceCell.density);
			densityL1 += densityError;
			densityLinf = std::max(densityLinf, densityError);
			const auto primitive = eos.ToPrimitive(final[cell]);
			if (primitive.valid)
				pressureLinf = std::max(pressureLinf, std::abs(primitive.pressure - 1.0));
		}
		summary.densityL1Error = densityL1 / static_cast<double>(final.size());
		summary.densityLinfError = densityLinf;
		summary.pressureLinfError = pressureLinf;
		const double initialVariation = DensityTotalVariation(initial);
		const double finalVariation = DensityTotalVariation(final);
		summary.totalVariationRatio = initialVariation > 0.0 ? finalVariation / initialVariation : 0.0;
		summary.densityBoundsPreserved = summary.minimumDensity >= minimumExpectedDensity - 1e-12
			&& summary.maximumDensity <= maximumExpectedDensity + 1e-12;
		summary.advectionReferencePassed = summary.densityL1Error <= densityL1Tolerance
			&& summary.densityLinfError <= densityLinfTolerance
			&& summary.pressureLinfError <= 1e-10
			&& summary.totalVariationRatio > 0.0
			&& summary.totalVariationRatio <= 1.0 + 1e-10
			&& summary.densityBoundsPreserved;
	}
}

RusanovProbeSummary RunRusanovUniform()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto initial = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0);
	return RunPeriodicProbe(
		{"rusanov_uniform_1d", {UniformCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, UniformTimeStep, UniformSteps},
		std::vector<ConservativeState>(UniformCells, initial), eos, false, false, 1e-12, nullptr);
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
		std::move(initial), eos, true, true, 1e-10, nullptr);
}

RusanovProbeSummary RunRusanovDensityAdvection()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(DensityAdvectionCells);
	for (std::size_t cell = 0; cell < DensityAdvectionCells; ++cell)
	{
		const double phase = 2.0 * Pi * static_cast<double>(cell)
			/ static_cast<double>(DensityAdvectionCells);
		const double density = 1.0 + DensityAdvectionAmplitude * std::sin(phase);
		initial[cell] = eos.FromPrimitive(
			density, DensityAdvectionVelocity, 0.0, 1.0);
	}
	std::vector<ConservativeState> final;
	auto summary = RunPeriodicProbe(
		{"rusanov_density_advection_1d",
			{DensityAdvectionCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, DensityAdvectionTimeStep, DensityAdvectionSteps},
		initial, eos, true, false, 1e-10, &final);
	EvaluateAdvectedDensity(summary, initial, final, eos, DensityAdvectionVelocity,
		0.01, 0.02, 1.0 - DensityAdvectionAmplitude, 1.0 + DensityAdvectionAmplitude);
	summary.passed = summary.passed && summary.advectionReferencePassed;
	return summary;
}

RusanovProbeSummary RunRusanovContactDiscontinuity()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(ContactCells);
	for (std::size_t cell = 0; cell < ContactCells; ++cell)
	{
		const double density = cell < ContactCells / 2
			? ContactHighDensity
			: ContactLowDensity;
		initial[cell] = eos.FromPrimitive(density, ContactVelocity, 0.0, 1.0);
	}
	std::vector<ConservativeState> final;
	auto summary = RunPeriodicProbe(
		{"rusanov_contact_discontinuity_1d",
			{ContactCells, 1, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, ContactTimeStep, ContactSteps},
		initial, eos, true, false, 1e-10, &final);
	EvaluateAdvectedDensity(summary, initial, final, eos, ContactVelocity,
		0.02, 0.2, ContactLowDensity, ContactHighDensity);
	summary.passed = summary.passed && summary.advectionReferencePassed;
	return summary;
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

bool WriteRusanovDensityAdvectionProbe(std::ostream &output)
{
	const auto summary = RunRusanovDensityAdvection();
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
	output << "reference_velocity=" << summary.referenceVelocity << '\n';
	output << "reference_shift_cells=" << summary.referenceShiftCells << '\n';
	output << "density_l1_error=" << summary.densityL1Error << '\n';
	output << "density_linf_error=" << summary.densityLinfError << '\n';
	output << "pressure_linf_error=" << summary.pressureLinfError << '\n';
	output << "total_variation_ratio=" << summary.totalVariationRatio << '\n';
	output << "advection_reference_passed="
		<< (summary.advectionReferencePassed ? "true" : "false") << '\n';
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

bool WriteRusanovContactDiscontinuityProbe(std::ostream &output)
{
	const auto summary = RunRusanovContactDiscontinuity();
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
	output << "reference_velocity=" << summary.referenceVelocity << '\n';
	output << "reference_shift_cells=" << summary.referenceShiftCells << '\n';
	output << "density_l1_error=" << summary.densityL1Error << '\n';
	output << "density_linf_error=" << summary.densityLinfError << '\n';
	output << "pressure_linf_error=" << summary.pressureLinfError << '\n';
	output << "total_variation_ratio=" << summary.totalVariationRatio << '\n';
	output << "density_bounds_preserved="
		<< (summary.densityBoundsPreserved ? "true" : "false") << '\n';
	output << "advection_reference_passed="
		<< (summary.advectionReferencePassed ? "true" : "false") << '\n';
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
	output << "maximum_density=" << summary.maximumDensity << '\n';
	output << "minimum_pressure=" << summary.minimumPressure << '\n';
	output << "minimum_energy_density=" << summary.minimumEnergyDensity << '\n';
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
