#include "LegacyLike.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <vector>

namespace omni::atmospherebench
{

namespace
{
	constexpr std::size_t CellsX = 64;
	constexpr std::size_t CellsY = 48;
	constexpr std::size_t UniformSteps = 32;
	constexpr std::size_t PulseSteps = 96;
	constexpr double PressureGradientScale = 0.08;
	constexpr double PressureDivergenceScale = 0.08;
	constexpr double PressureSmoothing = 0.015;
	constexpr double VelocitySmoothing = 0.01;
	constexpr double PulseWidthCells = 5.0;

	struct LegacyLikeCell
	{
		double pressure = 0.0;
		double velocityX = 0.0;
		double velocityY = 0.0;
		double airTemperature = 1.0;
	};

	double StateDifference(const LegacyLikeCell &left, const LegacyLikeCell &right)
	{
		return std::abs(left.pressure - right.pressure)
			+ std::abs(left.velocityX - right.velocityX)
			+ std::abs(left.velocityY - right.velocityY)
			+ std::abs(left.airTemperature - right.airTemperature);
	}

	bool Finite(const LegacyLikeCell &cell)
	{
		return std::isfinite(cell.pressure) && std::isfinite(cell.velocityX)
			&& std::isfinite(cell.velocityY) && std::isfinite(cell.airTemperature);
	}

	LegacyLikeProbeSummary RunProbe(
		const BenchmarkCase &benchmarkCase,
		std::vector<LegacyLikeCell> initial,
		bool requireUniform,
		bool requirePulseEvolution)
	{
		LegacyLikeProbeSummary summary{benchmarkCase};
		if (!benchmarkCase.IsValid() || benchmarkCase.grid.boundaryMode != BoundaryMode::Periodic
			|| benchmarkCase.grid.cellsX < 2 || benchmarkCase.grid.cellsY < 2
			|| initial.size() != benchmarkCase.grid.CellCount())
			return summary;
		std::vector<LegacyLikeCell> cells = initial;
		std::vector<LegacyLikeCell> next(cells.size());
		summary.stateAndScratchBytesTotal = 2 * cells.size() * sizeof(LegacyLikeCell);
		summary.stateAndScratchBytesPerCell = static_cast<double>(
			summary.stateAndScratchBytesTotal) / static_cast<double>(cells.size());
		summary.initialMaximumPressure = -std::numeric_limits<double>::infinity();
		for (const auto &cell : cells)
		{
			summary.initialPressureSum += cell.pressure;
			summary.initialVelocityXSum += cell.velocityX;
			summary.initialVelocityYSum += cell.velocityY;
			summary.initialMaximumPressure = std::max(
				summary.initialMaximumPressure, cell.pressure);
		}
		for (std::size_t step = 0; step < benchmarkCase.stepCount; ++step)
		{
			for (std::size_t y = 0; y < benchmarkCase.grid.cellsY; ++y)
			{
				for (std::size_t x = 0; x < benchmarkCase.grid.cellsX; ++x)
				{
					const std::size_t index = y * benchmarkCase.grid.cellsX + x;
					const std::size_t left = y * benchmarkCase.grid.cellsX
						+ (x + benchmarkCase.grid.cellsX - 1) % benchmarkCase.grid.cellsX;
					const std::size_t right = y * benchmarkCase.grid.cellsX
						+ (x + 1) % benchmarkCase.grid.cellsX;
					const std::size_t bottom = ((y + benchmarkCase.grid.cellsY - 1)
						% benchmarkCase.grid.cellsY) * benchmarkCase.grid.cellsX + x;
					const std::size_t top = ((y + 1) % benchmarkCase.grid.cellsY)
						* benchmarkCase.grid.cellsX + x;
					const double pressureGradientX = 0.5
						* (cells[right].pressure - cells[left].pressure);
					const double pressureGradientY = 0.5
						* (cells[top].pressure - cells[bottom].pressure);
					const double velocityDivergence = 0.5
						* ((cells[right].velocityX - cells[left].velocityX)
							+ (cells[top].velocityY - cells[bottom].velocityY));
					const double pressureLaplacian = cells[left].pressure + cells[right].pressure
						+ cells[bottom].pressure + cells[top].pressure - 4.0 * cells[index].pressure;
					const double velocityXLaplacian = cells[left].velocityX + cells[right].velocityX
						+ cells[bottom].velocityX + cells[top].velocityX - 4.0 * cells[index].velocityX;
					const double velocityYLaplacian = cells[left].velocityY + cells[right].velocityY
						+ cells[bottom].velocityY + cells[top].velocityY - 4.0 * cells[index].velocityY;
					next[index].pressure = cells[index].pressure
						- PressureDivergenceScale * velocityDivergence
						+ PressureSmoothing * pressureLaplacian;
					next[index].velocityX = cells[index].velocityX
						- PressureGradientScale * pressureGradientX
						+ VelocitySmoothing * velocityXLaplacian;
					next[index].velocityY = cells[index].velocityY
						- PressureGradientScale * pressureGradientY
						+ VelocitySmoothing * velocityYLaplacian;
					next[index].airTemperature = cells[index].airTemperature;
					if (!Finite(next[index]))
						return summary;
				}
			}
			cells.swap(next);
		}
		summary.minimumPressure = std::numeric_limits<double>::infinity();
		summary.finalMaximumPressure = -std::numeric_limits<double>::infinity();
		summary.finiteState = true;
		for (std::size_t index = 0; index < cells.size(); ++index)
		{
			if (!Finite(cells[index]))
				summary.finiteState = false;
			summary.finalPressureSum += cells[index].pressure;
			summary.finalVelocityXSum += cells[index].velocityX;
			summary.finalVelocityYSum += cells[index].velocityY;
			summary.minimumPressure = std::min(summary.minimumPressure, cells[index].pressure);
			summary.finalMaximumPressure = std::max(
				summary.finalMaximumPressure, cells[index].pressure);
			summary.maximumAbsoluteVelocity = std::max(summary.maximumAbsoluteVelocity,
				std::hypot(cells[index].velocityX, cells[index].velocityY));
			summary.stateChangeL1 += StateDifference(cells[index], initial[index]);
		}
		summary.pressureSumDrift = summary.finalPressureSum - summary.initialPressureSum;
		summary.uniformPreserved = summary.stateChangeL1 <= 1e-12;
		summary.stateEvolved = summary.stateChangeL1 > 1e-6;
		summary.pressurePeakReduced = summary.finalMaximumPressure
			< summary.initialMaximumPressure - 1e-6;
		summary.pressureSumPreserved = std::abs(summary.pressureSumDrift) <= 1e-10;
		summary.passed = summary.finiteState && summary.corrections.IsEmpty()
			&& summary.pressureSumPreserved
			&& (!requireUniform || summary.uniformPreserved)
			&& (!requirePulseEvolution
				|| (summary.stateEvolved && summary.pressurePeakReduced
					&& summary.maximumAbsoluteVelocity > 1e-6));
		return summary;
	}

	bool WriteProbe(
		std::ostream &output,
		const LegacyLikeProbeSummary &summary,
		bool pressurePulse)
	{
		output << "schema_version=1\n";
		output << "case=" << summary.benchmarkCase.id << '\n';
		output << "candidate=legacy_like\n";
		output << "candidate_solver_implemented=true\n";
		output << "atmosphere_solver_selection=unselected\n";
		output << "physical_scale_selection=unselected\n";
		output << "result_status=control_result_not_solver_selection\n";
		output << "case_time_domain=nondimensional_contract\n";
		output << "dimension=2\n";
		output << "boundary_mode=periodic\n";
		output << "control_model=dimensionless_pressure_velocity_stencil\n";
		output << "production_air_equivalence=not_claimed\n";
		output << "physical_mass_state=not_implemented\n";
		output << "physical_density_state=not_implemented\n";
		output << "physical_momentum_state=not_implemented\n";
		output << "physical_energy_state=not_implemented\n";
		output << "species_state=not_implemented\n";
		output << "mass_conservation=not_applicable_no_mass_state\n";
		output << "momentum_conservation=not_applicable_no_momentum_density_state\n";
		output << "energy_conservation=not_applicable_no_energy_state\n";
		output << "grid_cells_x=" << summary.benchmarkCase.grid.cellsX << '\n';
		output << "grid_cells_y=" << summary.benchmarkCase.grid.cellsY << '\n';
		output << "grid_cell_count=" << summary.benchmarkCase.grid.CellCount() << '\n';
		output << "cell_length=" << summary.benchmarkCase.grid.cellLength << '\n';
		output << "case_timestep=" << summary.benchmarkCase.timeStep << '\n';
		output << "case_step_count=" << summary.benchmarkCase.stepCount << '\n';
		output << "maximum_cfl=not_applicable_legacy_dimensionless_stencil\n";
		output << "minimum_density=not_applicable_no_density_state\n";
		output << "minimum_pressure=" << summary.minimumPressure << '\n';
		output << "initial_maximum_pressure=" << summary.initialMaximumPressure << '\n';
		output << "final_maximum_pressure=" << summary.finalMaximumPressure << '\n';
		output << "initial_pressure_sum=" << summary.initialPressureSum << '\n';
		output << "final_pressure_sum=" << summary.finalPressureSum << '\n';
		output << "pressure_sum_drift=" << summary.pressureSumDrift << '\n';
		output << "initial_velocity_x_sum=" << summary.initialVelocityXSum << '\n';
		output << "final_velocity_x_sum=" << summary.finalVelocityXSum << '\n';
		output << "initial_velocity_y_sum=" << summary.initialVelocityYSum << '\n';
		output << "final_velocity_y_sum=" << summary.finalVelocityYSum << '\n';
		output << "maximum_absolute_velocity=" << summary.maximumAbsoluteVelocity << '\n';
		output << "mass_drift=not_applicable_no_mass_state\n";
		output << "momentum_x_drift=not_applicable_no_momentum_density_state\n";
		output << "momentum_y_drift=not_applicable_no_momentum_density_state\n";
		output << "momentum_drift=not_applicable_no_momentum_density_state\n";
		output << "energy_drift=not_applicable_no_energy_state\n";
		output << "state_change_l1=" << summary.stateChangeL1 << '\n';
		output << "state_evolved=" << (summary.stateEvolved ? "true" : "false") << '\n';
		output << "uniform_preserved=" << (summary.uniformPreserved ? "true" : "false") << '\n';
		output << "pressure_peak_reduced="
			<< (summary.pressurePeakReduced ? "true" : "false") << '\n';
		output << "pressure_sum_preserved="
			<< (summary.pressureSumPreserved ? "true" : "false") << '\n';
		output << "finite_state=" << (summary.finiteState ? "true" : "false") << '\n';
		output << "probe_kind="
			<< (pressurePulse ? "pressure_pulse" : "uniform_preservation") << '\n';
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
		output << "state_bytes_per_cell=" << sizeof(LegacyLikeCell) << '\n';
		output << "state_and_flux_scratch_bytes_per_cell="
			<< summary.stateAndScratchBytesPerCell << '\n';
		output << "state_and_flux_scratch_bytes_total="
			<< summary.stateAndScratchBytesTotal << '\n';
		output << "probe_passed=" << (summary.passed ? "true" : "false") << '\n';
		return summary.passed;
	}
} // namespace

LegacyLikeProbeSummary RunLegacyLikeUniform()
{
	return RunProbe(
		{"legacy_like_uniform_2d",
			{CellsX, CellsY, 1.0, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, 1.0, UniformSteps},
		std::vector<LegacyLikeCell>(CellsX * CellsY), true, false);
}

LegacyLikeProbeSummary RunLegacyLikePressurePulse()
{
	std::vector<LegacyLikeCell> initial(CellsX * CellsY);
	const double centerX = 0.5 * static_cast<double>(CellsX);
	const double centerY = 0.5 * static_cast<double>(CellsY);
	for (std::size_t y = 0; y < CellsY; ++y)
	{
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const double dx = static_cast<double>(x) + 0.5 - centerX;
			const double dy = static_cast<double>(y) + 0.5 - centerY;
			initial[y * CellsX + x].pressure = std::exp(
				-(dx * dx + dy * dy) / (2.0 * PulseWidthCells * PulseWidthCells));
		}
	}
	return RunProbe(
		{"legacy_like_pressure_pulse_2d",
			{CellsX, CellsY, 1.0, BoundaryMode::Periodic},
			TimeDomain::NondimensionalContract, 1.0, PulseSteps},
		std::move(initial), false, true);
}

bool WriteLegacyLikeUniformProbe(std::ostream &output)
{
	return WriteProbe(output, RunLegacyLikeUniform(), false);
}

bool WriteLegacyLikePressurePulseProbe(std::ostream &output)
{
	return WriteProbe(output, RunLegacyLikePressurePulse(), true);
}

} // namespace omni::atmospherebench
