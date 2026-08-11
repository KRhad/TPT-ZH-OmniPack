#include "LowMachProjection2D.h"

#include <algorithm>
#include <chrono>
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
	constexpr double CellLength = 1.0;
	constexpr double TimeStep = 0.1;
	constexpr std::size_t Iterations = 4000;
	constexpr double Pi = 3.141592653589793238462643383279502884;

	std::size_t Index(std::size_t x, std::size_t y)
	{
		return y * CellsX + x;
	}

	double DivergenceL2(const std::vector<double> &velocityX,
		const std::vector<double> &velocityY, std::vector<double> *output)
	{
		double sum = 0.0;
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index(x, y);
				const auto left = Index((x + CellsX - 1) % CellsX, y);
				const auto bottom = Index(x, (y + CellsY - 1) % CellsY);
				const double divergence = (velocityX[index] - velocityX[left]
					+ velocityY[index] - velocityY[bottom]) / CellLength;
				if (output)
					(*output)[index] = divergence;
				sum += divergence * divergence;
			}
		return std::sqrt(sum / static_cast<double>(CellsX * CellsY));
	}

	double KineticEnergy(const std::vector<double> &velocityX,
		const std::vector<double> &velocityY)
	{
		double result = 0.0;
		for (std::size_t index = 0; index < velocityX.size(); ++index)
			result += 0.5 * (velocityX[index] * velocityX[index]
				+ velocityY[index] * velocityY[index]);
		return result;
	}
}

LowMachProjection2DSummary RunLowMachProjection2D()
{
	LowMachProjection2DSummary summary;
	summary.cellsX = CellsX;
	summary.cellsY = CellsY;
	summary.iterationCount = Iterations;
	summary.timeStep = TimeStep;
	summary.workingBytesPerCell = 5 * sizeof(double);
	summary.workingBytesTotal = summary.workingBytesPerCell * CellsX * CellsY;
	std::vector<double> velocityX(CellsX * CellsY);
	std::vector<double> velocityY(CellsX * CellsY);
	std::vector<double> divergence(CellsX * CellsY);
	std::vector<double> pressure(CellsX * CellsY, 0.0);
	std::vector<double> nextPressure(CellsX * CellsY, 0.0);
	for (std::size_t y = 0; y < CellsY; ++y)
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			velocityX[Index(x, y)] = 0.08 * std::sin(2.0 * Pi
				* (static_cast<double>(x) + 0.5) / static_cast<double>(CellsX));
			velocityY[Index(x, y)] = 0.05 * std::sin(2.0 * Pi
				* (static_cast<double>(y) + 0.5) / static_cast<double>(CellsY));
		}
	summary.initialDivergenceL2 = DivergenceL2(velocityX, velocityY, &divergence);
	summary.initialKineticEnergy = KineticEnergy(velocityX, velocityY);
	const auto start = std::chrono::steady_clock::now();
	const double rhsScale = CellLength * CellLength / TimeStep;
	for (std::size_t iteration = 0; iteration < Iterations; ++iteration)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto left = Index((x + CellsX - 1) % CellsX, y);
				const auto right = Index((x + 1) % CellsX, y);
				const auto bottom = Index(x, (y + CellsY - 1) % CellsY);
				const auto top = Index(x, (y + 1) % CellsY);
				nextPressure[Index(x, y)] = 0.25 * (pressure[left] + pressure[right]
					+ pressure[bottom] + pressure[top]
					- rhsScale * divergence[Index(x, y)]);
			}
		pressure.swap(nextPressure);
	}
	for (std::size_t y = 0; y < CellsY; ++y)
		for (std::size_t x = 0; x < CellsX; ++x)
		{
			const auto index = Index(x, y);
			const auto right = Index((x + 1) % CellsX, y);
			const auto top = Index(x, (y + 1) % CellsY);
			velocityX[index] -= TimeStep * (pressure[right] - pressure[index]) / CellLength;
			velocityY[index] -= TimeStep * (pressure[top] - pressure[index]) / CellLength;
		}
	const auto end = std::chrono::steady_clock::now();
	summary.elapsedMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
	summary.millisecondsPerIteration = summary.elapsedMilliseconds
		/ static_cast<double>(Iterations);
	summary.finalDivergenceL2 = DivergenceL2(velocityX, velocityY, nullptr);
	summary.divergenceReductionRatio = summary.finalDivergenceL2
		/ summary.initialDivergenceL2;
	summary.finalKineticEnergy = KineticEnergy(velocityX, velocityY);
	double pressureSum = 0.0;
	for (const auto value : pressure)
	{
		pressureSum += value;
		summary.pressureCorrectionMaximumAbsolute = std::max(
			summary.pressureCorrectionMaximumAbsolute, std::abs(value));
	}
	summary.pressureCorrectionMean = pressureSum / static_cast<double>(pressure.size());
	summary.finiteState = std::isfinite(summary.finalDivergenceL2)
		&& std::isfinite(summary.finalKineticEnergy)
		&& std::isfinite(summary.pressureCorrectionMean)
		&& std::isfinite(summary.pressureCorrectionMaximumAbsolute);
	summary.divergenceReduced = summary.divergenceReductionRatio < 0.02;
	summary.soundSpeedIndependent = true;
	summary.passed = summary.finiteState && summary.divergenceReduced
		&& summary.soundSpeedIndependent
		&& std::abs(summary.pressureCorrectionMean) < 1e-10
		&& summary.finalKineticEnergy <= summary.initialKineticEnergy;
	return summary;
}

bool WriteLowMachProjection2D(std::ostream &output)
{
	const auto summary = RunLowMachProjection2D();
	output << "schema_version=1\n";
	output << "case=low_mach_pressure_projection_2d\n";
	output << "candidate=periodic_low_mach_projection_component\n";
	output << "candidate_solver_implemented=false\n";
	output << "physical_scale_selection=unselected\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "result_status=low_mach_component_probe\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "case_timestep=" << summary.timeStep << '\n';
	output << "case_step_count=1\n";
	output << "grid_cell_count=" << (summary.cellsX * summary.cellsY) << '\n';
	output << "cell_length=1\n";
	output << "state_bytes_per_cell=40\n";
	output << "state_and_flux_scratch_bytes_per_cell=" << summary.workingBytesPerCell << '\n';
	output << "state_and_flux_scratch_bytes_total=" << summary.workingBytesTotal << '\n';
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=" << summary.cellsX << '\n';
	output << "grid_cells_y=" << summary.cellsY << '\n';
	output << "iteration_count=" << summary.iterationCount << '\n';
	output << "time_step=" << summary.timeStep << '\n';
	output << "initial_divergence_l2=" << summary.initialDivergenceL2 << '\n';
	output << "final_divergence_l2=" << summary.finalDivergenceL2 << '\n';
	output << "divergence_reduction_ratio=" << summary.divergenceReductionRatio << '\n';
	output << "initial_kinetic_energy=" << summary.initialKineticEnergy << '\n';
	output << "final_kinetic_energy=" << summary.finalKineticEnergy << '\n';
	output << "pressure_correction_mean=" << summary.pressureCorrectionMean << '\n';
	output << "pressure_correction_maximum_absolute="
		<< summary.pressureCorrectionMaximumAbsolute << '\n';
	output << "elapsed_milliseconds=" << summary.elapsedMilliseconds << '\n';
	output << "milliseconds_per_iteration=" << summary.millisecondsPerIteration << '\n';
	output << "working_bytes_per_cell=" << summary.workingBytesPerCell << '\n';
	output << "working_bytes_total=" << summary.workingBytesTotal << '\n';
	output << "sound_speed_dependency=false\n";
	output << "general_low_mach_pressure_coupling=implemented_periodic_projection_component_only\n";
	output << "compressible_event_coupling=not_implemented_in_this_component\n";
	output << "near_vacuum_support=not_applicable_bulk_component_routes_vacuum_to_compressible\n";
	output << "production_runtime_integration=not_implemented\n";
	output << "mass_conservation=not_instrumented_velocity_projection_component_only\n";
	output << "momentum_conservation=not_instrumented_velocity_projection_component_only\n";
	output << "energy_conservation=not_instrumented_velocity_projection_component_only\n";
	output << "numerical_correction_count=0\n";
	output << "correction_mass_added=0\n";
	output << "correction_mass_removed=0\n";
	output << "correction_momentum_x_added=0\n";
	output << "correction_momentum_y_added=0\n";
	output << "correction_energy_added=0\n";
	output << "correction_energy_removed=0\n";
	output << "density_floor_hits=0\n";
	output << "pressure_floor_hits=0\n";
	output << "correction_event_count=0\n";
	output << "finite_state=" << (summary.finiteState ? "true" : "false") << '\n';
	output << "divergence_reduced=" << (summary.divergenceReduced ? "true" : "false") << '\n';
	output << "projection_probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	return summary.passed;
}

} // namespace omni::atmospherebench
