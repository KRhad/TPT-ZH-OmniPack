#include "Hllc2D.h"
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
	constexpr std::size_t CellsX = 32;
	constexpr std::size_t CellsY = 24;
	constexpr double CellLength = 1.0;
	constexpr double UniformTimeStep = 0.02;
	constexpr std::size_t UniformSteps = 8;
	constexpr double PulseTimeStep = 0.01;
	constexpr std::size_t PulseSteps = 40;
	constexpr double PulseAmplitude = 0.1;
	constexpr double PulseWidthCells = 4.0;

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

	double StateChangeL1(
		const std::vector<ConservativeState> &initial,
		const std::vector<ConservativeState> &final)
	{
		if (initial.size() != final.size())
			return std::numeric_limits<double>::quiet_NaN();
		double result = 0.0;
		for (std::size_t index = 0; index < initial.size(); ++index)
		{
			const auto difference = Subtract(final[index], initial[index]);
			result += std::abs(difference.density) + std::abs(difference.momentumX)
				+ std::abs(difference.momentumY) + std::abs(difference.totalEnergyDensity);
		}
		return result;
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

	NumericalFluxResult FluxY(
		const ConservativeState &bottom,
		const ConservativeState &top,
		const IdealGasEOS &eos)
	{
		const ConservativeState rotatedBottom{
			bottom.density, bottom.momentumY, bottom.momentumX, bottom.totalEnergyDensity};
		const ConservativeState rotatedTop{
			top.density, top.momentumY, top.momentumX, top.totalEnergyDensity};
		auto result = ComputeHllcRusanovFallbackFluxX(rotatedBottom, rotatedTop, eos);
		std::swap(result.flux.momentumX, result.flux.momentumY);
		return result;
	}

	Hllc2DProbeSummary RunPeriodicProbe(
		const BenchmarkCase &benchmarkCase,
		std::vector<ConservativeState> initial,
		const IdealGasEOS &eos,
		bool requireEvolution,
		bool requirePeakReduction,
		double conservationTolerance)
	{
		Hllc2DProbeSummary summary{benchmarkCase};
		if (!benchmarkCase.IsValid() || benchmarkCase.grid.boundaryMode != BoundaryMode::Periodic
			|| benchmarkCase.grid.cellsX < 2 || benchmarkCase.grid.cellsY < 2
			|| initial.size() != benchmarkCase.grid.CellCount() || !eos.IsValid())
			return summary;
		std::vector<ConservativeState> cells = initial;
		std::vector<ConservativeState> next(cells.size());
		std::vector<ConservativeState> fluxX(cells.size());
		std::vector<ConservativeState> fluxY(cells.size());
		summary.ledger.Begin(TotalState(cells));
		summary.initialMaximumPressure = MaximumPressure(cells, eos);
		summary.minimumDensity = std::numeric_limits<double>::infinity();
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		const double lambda = benchmarkCase.timeStep / benchmarkCase.grid.cellLength;
		for (std::size_t step = 0; step < benchmarkCase.stepCount; ++step)
		{
			for (std::size_t y = 0; y < benchmarkCase.grid.cellsY; ++y)
			{
				for (std::size_t x = 0; x < benchmarkCase.grid.cellsX; ++x)
				{
					const std::size_t index = y * benchmarkCase.grid.cellsX + x;
					const std::size_t right = y * benchmarkCase.grid.cellsX
						+ (x + 1) % benchmarkCase.grid.cellsX;
					const std::size_t top = ((y + 1) % benchmarkCase.grid.cellsY)
						* benchmarkCase.grid.cellsX + x;
					const auto xResult = ComputeHllcRusanovFallbackFluxX(cells[index], cells[right], eos);
					const auto yResult = FluxY(cells[index], cells[top], eos);
					if (!xResult.valid || !yResult.valid)
						return summary;
					fluxX[index] = xResult.flux;
					fluxY[index] = yResult.flux;
					summary.fluxFallbackCount += static_cast<std::size_t>(xResult.usedFallback)
						+ static_cast<std::size_t>(yResult.usedFallback);
				}
			}
			for (std::size_t y = 0; y < benchmarkCase.grid.cellsY; ++y)
			{
				for (std::size_t x = 0; x < benchmarkCase.grid.cellsX; ++x)
				{
					const std::size_t index = y * benchmarkCase.grid.cellsX + x;
					const std::size_t left = y * benchmarkCase.grid.cellsX
						+ (x + benchmarkCase.grid.cellsX - 1) % benchmarkCase.grid.cellsX;
					const std::size_t bottom = ((y + benchmarkCase.grid.cellsY - 1)
						% benchmarkCase.grid.cellsY) * benchmarkCase.grid.cellsX + x;
					next[index] = Subtract(cells[index], Scale(lambda,
						Add(Subtract(fluxX[index], fluxX[left]),
							Subtract(fluxY[index], fluxY[bottom]))));
					const auto primitive = eos.ToPrimitive(next[index]);
					if (!primitive.valid)
						return summary;
					summary.minimumDensity = std::min(summary.minimumDensity, primitive.density);
					summary.minimumPressure = std::min(summary.minimumPressure, primitive.pressure);
					summary.maximumCfl = std::max(summary.maximumCfl, lambda
						* (std::abs(primitive.velocityX) + std::abs(primitive.velocityY)
							+ 2.0 * primitive.soundSpeed));
				}
			}
			cells.swap(next);
		}
		summary.finalMaximumPressure = MaximumPressure(cells, eos);
		summary.stateChangeL1 = StateChangeL1(initial, cells);
		summary.stateEvolved = std::isfinite(summary.stateChangeL1) && summary.stateChangeL1 > 1e-12;
		summary.pressurePeakReduced = summary.finalMaximumPressure < summary.initialMaximumPressure;
		summary.positivityPreserved = std::isfinite(summary.minimumDensity)
			&& std::isfinite(summary.minimumPressure)
			&& summary.minimumDensity > 0.0 && summary.minimumPressure > 0.0;
		summary.ledger.End(TotalState(cells));
		summary.passed = summary.positivityPreserved
			&& summary.ledger.Closes(conservationTolerance)
			&& summary.corrections.IsEmpty() && summary.maximumCfl <= 1.0
			&& (!requireEvolution || summary.stateEvolved)
			&& (!requirePeakReduction || summary.pressurePeakReduced);
		return summary;
	}

	bool WriteProbe(std::ostream &output, const Hllc2DProbeSummary &summary)
	{
		const auto &initial = summary.ledger.initial;
		const auto &final = summary.ledger.final;
		output << "schema_version=1\n";
		output << "case=" << summary.benchmarkCase.id << '\n';
		output << "candidate=fvm_hllc_rusanov_fallback\n";
		output << "candidate_solver_implemented=true\n";
		output << "atmosphere_solver_selection=unselected\n";
		output << "physical_scale_selection=unselected\n";
		output << "result_status=candidate_result_not_selection\n";
		output << "case_time_domain=nondimensional_contract\n";
		output << "dimension=2\n";
		output << "boundary_mode=periodic\n";
		output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
		output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
		output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
		output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
		output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
		output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
		output << "maximum_cfl=" << summary.maximumCfl << '\n';
		output << "minimum_density=" << summary.minimumDensity << '\n';
		output << "minimum_pressure=" << summary.minimumPressure << '\n';
		output << "initial_maximum_pressure=" << summary.initialMaximumPressure << '\n';
		output << "final_maximum_pressure=" << summary.finalMaximumPressure << '\n';
		output << "mass_drift=" << (final.density - initial.density) << '\n';
		output << "momentum_drift=" << std::hypot(
			final.momentumX - initial.momentumX,
			final.momentumY - initial.momentumY) << '\n';
		output << "momentum_x_drift=" << (final.momentumX - initial.momentumX) << '\n';
		output << "momentum_y_drift=" << (final.momentumY - initial.momentumY) << '\n';
		output << "energy_drift=" << (final.totalEnergyDensity - initial.totalEnergyDensity) << '\n';
		output << "state_change_l1=" << summary.stateChangeL1 << '\n';
		output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
		output << "pressure_peak_reduced=" << (summary.pressurePeakReduced ? "true" : "false") << '\n';
		output << "positivity_preserved=" << (summary.positivityPreserved ? "true" : "false") << '\n';
		output << "flux_fallback_count=" << summary.fluxFallbackCount << '\n';
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
		output << "state_and_flux_scratch_bytes_per_cell=" << (5 * sizeof(ConservativeState)) << '\n';
		output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
		return summary.passed;
	}
} // namespace

Hllc2DProbeSummary RunHllc2DUniform()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	const auto state = eos.FromPrimitive(1.0, 0.1, -0.05, 1.0);
	return RunPeriodicProbe(
		{"hllc_uniform_2d", {CellsX, CellsY, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, UniformTimeStep, UniformSteps},
		std::vector<ConservativeState>(CellsX * CellsY, state), eos, false, false, 1e-10);
}

Hllc2DProbeSummary RunHllc2DPressurePulse()
{
	const IdealGasEOS eos(Gamma, SpecificGasConstant);
	std::vector<ConservativeState> initial(CellsX * CellsY);
	const double centerX = 0.5 * static_cast<double>(CellsX);
	const double centerY = 0.5 * static_cast<double>(CellsY);
	for (std::size_t y = 0; y < CellsY; ++y)
	{
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const double dx = static_cast<double>(x) + 0.5 - centerX;
			const double dy = static_cast<double>(y) + 0.5 - centerY;
			const double pulse = PulseAmplitude * std::exp(
				-(dx * dx + dy * dy) / (2.0 * PulseWidthCells * PulseWidthCells));
			initial[y * CellsX + x] = eos.FromPrimitive(1.0, 0.0, 0.0, 1.0 + pulse);
		}
	}
	return RunPeriodicProbe(
		{"hllc_pressure_pulse_2d", {CellsX, CellsY, CellLength, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, PulseTimeStep, PulseSteps},
		std::move(initial), eos, true, true, 1e-9);
}

bool WriteHllc2DUniformProbe(std::ostream &output)
{
	return WriteProbe(output, RunHllc2DUniform());
}

bool WriteHllc2DPressurePulseProbe(std::ostream &output)
{
	return WriteProbe(output, RunHllc2DPressurePulse());
}

} // namespace omni::atmospherebench
