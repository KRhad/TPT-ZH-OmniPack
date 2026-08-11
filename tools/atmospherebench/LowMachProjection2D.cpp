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
	constexpr std::size_t JacobiIterations = 4000;
	constexpr std::size_t ConjugateGradientMaximumIterations = 500;
	constexpr double ConjugateGradientTolerance = 1e-10;
	constexpr double ReferenceAtmosphereBudgetMilliseconds = 1000.0 / 60.0 / 4.0;
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

	void InitializeVelocity(std::vector<double> &velocityX,
		std::vector<double> &velocityY)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const double normalizedX = static_cast<double>(x) + 0.5;
				const double normalizedY = static_cast<double>(y) + 0.5;
				velocityX[Index(x, y)] =
					0.080 * std::sin(2.0 * Pi * normalizedX / CellsX)
					+ 0.023 * std::sin(6.0 * Pi * normalizedX / CellsX)
					+ 0.017 * std::cos(10.0 * Pi * normalizedX / CellsX)
					+ 0.011 * std::sin(2.0 * Pi * (normalizedX + normalizedY) / CellsX);
				velocityY[Index(x, y)] =
					0.050 * std::sin(2.0 * Pi * normalizedY / CellsY)
					+ 0.019 * std::sin(8.0 * Pi * normalizedY / CellsY)
					+ 0.013 * std::cos(12.0 * Pi * normalizedY / CellsY)
					+ 0.009 * std::cos(2.0 * Pi * (normalizedX - normalizedY) / CellsY);
			}
	}

	void ApplyPressureCorrection(std::vector<double> &velocityX,
		std::vector<double> &velocityY, const std::vector<double> &pressure)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index(x, y);
				const auto right = Index((x + 1) % CellsX, y);
				const auto top = Index(x, (y + 1) % CellsY);
				velocityX[index] -= TimeStep * (pressure[right] - pressure[index]) / CellLength;
				velocityY[index] -= TimeStep * (pressure[top] - pressure[index]) / CellLength;
			}
	}

	double PressureMean(const std::vector<double> &pressure,
		double &maximumAbsolute)
	{
		double sum = 0.0;
		maximumAbsolute = 0.0;
		for (const double value : pressure)
		{
			sum += value;
			maximumAbsolute = std::max(maximumAbsolute, std::abs(value));
		}
		return sum / static_cast<double>(pressure.size());
	}

	void ApplyPositivePoissonOperator(const std::vector<double> &input,
		std::vector<double> &output)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index(x, y);
				const auto left = Index((x + CellsX - 1) % CellsX, y);
				const auto right = Index((x + 1) % CellsX, y);
				const auto bottom = Index(x, (y + CellsY - 1) % CellsY);
				const auto top = Index(x, (y + 1) % CellsY);
				output[index] = (4.0 * input[index] - input[left] - input[right]
					- input[bottom] - input[top]) / (CellLength * CellLength);
			}
	}

	struct ProjectionRun
	{
		std::size_t iterationCount = 0;
		double elapsedMilliseconds = 0.0;
		double finalDivergenceL2 = 0.0;
		double divergenceReductionRatio = 0.0;
		double finalKineticEnergy = 0.0;
		double pressureMean = 0.0;
		double pressureMaximumAbsolute = 0.0;
		bool finiteState = false;
		bool divergenceReduced = false;
		bool passed = false;
	};

	ProjectionRun RunJacobi(const std::vector<double> &initialVelocityX,
		const std::vector<double> &initialVelocityY, std::vector<double> &pressure)
	{
		ProjectionRun result;
		std::vector<double> velocityX = initialVelocityX;
		std::vector<double> velocityY = initialVelocityY;
		std::vector<double> divergence(CellsX * CellsY);
		std::vector<double> nextPressure(CellsX * CellsY, 0.0);
		const double initialDivergence = DivergenceL2(velocityX, velocityY, &divergence);
		const auto start = std::chrono::steady_clock::now();
		const double rhsScale = CellLength * CellLength / TimeStep;
		for (std::size_t iteration = 0; iteration < JacobiIterations; ++iteration)
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
		ApplyPressureCorrection(velocityX, velocityY, pressure);
		const auto end = std::chrono::steady_clock::now();
		result.iterationCount = JacobiIterations;
		result.elapsedMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
		result.finalDivergenceL2 = DivergenceL2(velocityX, velocityY, nullptr);
		result.divergenceReductionRatio = result.finalDivergenceL2 / initialDivergence;
		result.finalKineticEnergy = KineticEnergy(velocityX, velocityY);
		result.pressureMean = PressureMean(pressure, result.pressureMaximumAbsolute);
		result.finiteState = std::isfinite(result.finalDivergenceL2)
			&& std::isfinite(result.finalKineticEnergy)
			&& std::isfinite(result.pressureMean)
			&& std::isfinite(result.pressureMaximumAbsolute);
		result.divergenceReduced = result.divergenceReductionRatio < 0.02;
		result.passed = result.finiteState && result.divergenceReduced
			&& std::abs(result.pressureMean) < 1e-10
			&& result.finalKineticEnergy <= KineticEnergy(initialVelocityX, initialVelocityY);
		return result;
	}

	ProjectionRun RunConjugateGradient(const std::vector<double> &initialVelocityX,
		const std::vector<double> &initialVelocityY, std::vector<double> &pressure)
	{
		ProjectionRun result;
		std::vector<double> velocityX = initialVelocityX;
		std::vector<double> velocityY = initialVelocityY;
		std::vector<double> divergence(CellsX * CellsY);
		std::vector<double> residual(CellsX * CellsY);
		std::vector<double> direction(CellsX * CellsY);
		std::vector<double> operatorValue(CellsX * CellsY);
		const double initialDivergence = DivergenceL2(velocityX, velocityY, &divergence);
		for (std::size_t index = 0; index < pressure.size(); ++index)
		{
			pressure[index] = 0.0;
			residual[index] = -divergence[index] / TimeStep;
			direction[index] = residual[index];
		}
		double residualSquared = 0.0;
		for (const double value : residual)
			residualSquared += value * value;
		const auto start = std::chrono::steady_clock::now();
		for (std::size_t iteration = 0; iteration < ConjugateGradientMaximumIterations;
			++iteration)
		{
			ApplyPositivePoissonOperator(direction, operatorValue);
			double directionOperator = 0.0;
			for (std::size_t index = 0; index < direction.size(); ++index)
				directionOperator += direction[index] * operatorValue[index];
			if (!(directionOperator > 0.0) || !std::isfinite(directionOperator))
				break;
			const double alpha = residualSquared / directionOperator;
			for (std::size_t index = 0; index < pressure.size(); ++index)
			{
				pressure[index] += alpha * direction[index];
				residual[index] -= alpha * operatorValue[index];
			}
			result.iterationCount = iteration + 1;
			double nextResidualSquared = 0.0;
			for (const double value : residual)
				nextResidualSquared += value * value;
			if (std::sqrt(nextResidualSquared / residual.size()) < ConjugateGradientTolerance)
				break;
			const double beta = nextResidualSquared / residualSquared;
			for (std::size_t index = 0; index < direction.size(); ++index)
				direction[index] = residual[index] + beta * direction[index];
			residualSquared = nextResidualSquared;
		}
		ApplyPressureCorrection(velocityX, velocityY, pressure);
		const auto end = std::chrono::steady_clock::now();
		result.elapsedMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
		result.finalDivergenceL2 = DivergenceL2(velocityX, velocityY, nullptr);
		result.divergenceReductionRatio = result.finalDivergenceL2 / initialDivergence;
		result.finalKineticEnergy = KineticEnergy(velocityX, velocityY);
		result.pressureMean = PressureMean(pressure, result.pressureMaximumAbsolute);
		result.finiteState = std::isfinite(result.finalDivergenceL2)
			&& std::isfinite(result.finalKineticEnergy)
			&& std::isfinite(result.pressureMean)
			&& std::isfinite(result.pressureMaximumAbsolute)
			&& result.iterationCount > 0;
		result.divergenceReduced = result.divergenceReductionRatio < 0.02;
		result.passed = result.finiteState && result.divergenceReduced
			&& std::abs(result.pressureMean) < 1e-10
			&& result.finalKineticEnergy <= KineticEnergy(initialVelocityX, initialVelocityY);
		return result;
	}
}

LowMachProjection2DSummary RunLowMachProjection2D()
{
	LowMachProjection2DSummary summary;
	summary.cellsX = CellsX;
	summary.cellsY = CellsY;
	summary.iterationCount = JacobiIterations;
	summary.timeStep = TimeStep;
	summary.workingBytesPerCell = 5 * sizeof(double);
	summary.workingBytesTotal = summary.workingBytesPerCell * CellsX * CellsY;
	std::vector<double> initialVelocityX(CellsX * CellsY);
	std::vector<double> initialVelocityY(CellsX * CellsY);
	std::vector<double> jacobiPressure(CellsX * CellsY, 0.0);
	std::vector<double> conjugateGradientPressure(CellsX * CellsY, 0.0);
	InitializeVelocity(initialVelocityX, initialVelocityY);
	const double initialKineticEnergy = KineticEnergy(initialVelocityX, initialVelocityY);
	const auto jacobi = RunJacobi(initialVelocityX, initialVelocityY, jacobiPressure);
	const auto conjugateGradient = RunConjugateGradient(initialVelocityX, initialVelocityY,
		conjugateGradientPressure);
	summary.initialDivergenceL2 = DivergenceL2(initialVelocityX, initialVelocityY, nullptr);
	summary.finalDivergenceL2 = jacobi.finalDivergenceL2;
	summary.divergenceReductionRatio = jacobi.divergenceReductionRatio;
	summary.initialKineticEnergy = initialKineticEnergy;
	summary.finalKineticEnergy = jacobi.finalKineticEnergy;
	summary.pressureCorrectionMean = jacobi.pressureMean;
	summary.pressureCorrectionMaximumAbsolute = jacobi.pressureMaximumAbsolute;
	summary.elapsedMilliseconds = jacobi.elapsedMilliseconds;
	summary.millisecondsPerIteration = jacobi.elapsedMilliseconds
		/ static_cast<double>(JacobiIterations);
	summary.finiteState = jacobi.finiteState && conjugateGradient.finiteState;
	summary.divergenceReduced = jacobi.divergenceReduced && conjugateGradient.divergenceReduced;
	summary.soundSpeedIndependent = true;
	summary.jacobiPassed = jacobi.passed;
	summary.conjugateGradientIterationCount = conjugateGradient.iterationCount;
	summary.conjugateGradientFinalDivergenceL2 = conjugateGradient.finalDivergenceL2;
	summary.conjugateGradientDivergenceReductionRatio = conjugateGradient.divergenceReductionRatio;
	summary.conjugateGradientElapsedMilliseconds = conjugateGradient.elapsedMilliseconds;
	summary.conjugateGradientMillisecondsPerIteration = conjugateGradient.iterationCount > 0
		? conjugateGradient.elapsedMilliseconds / static_cast<double>(conjugateGradient.iterationCount) : 0.0;
	summary.conjugateGradientFiniteState = conjugateGradient.finiteState;
	summary.conjugateGradientPassed = conjugateGradient.passed;
	summary.conjugateGradientWithinReferenceBudget = conjugateGradient.elapsedMilliseconds
		<= ReferenceAtmosphereBudgetMilliseconds;
	summary.passed = summary.jacobiPassed && summary.conjugateGradientPassed;
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
	output << "pressure_correction_maximum_absolute=" << summary.pressureCorrectionMaximumAbsolute << '\n';
	output << "elapsed_milliseconds=" << summary.elapsedMilliseconds << '\n';
	output << "milliseconds_per_iteration=" << summary.millisecondsPerIteration << '\n';
	output << "jacobi_iteration_count=" << summary.iterationCount << '\n';
	output << "jacobi_probe_passed=" << (summary.jacobiPassed ? "true" : "false") << '\n';
	output << "conjugate_gradient_iteration_count=" << summary.conjugateGradientIterationCount << '\n';
	output << "conjugate_gradient_final_divergence_l2=" << summary.conjugateGradientFinalDivergenceL2 << '\n';
	output << "conjugate_gradient_divergence_reduction_ratio=" << summary.conjugateGradientDivergenceReductionRatio << '\n';
	output << "conjugate_gradient_elapsed_milliseconds=" << summary.conjugateGradientElapsedMilliseconds << '\n';
	output << "conjugate_gradient_milliseconds_per_iteration=" << summary.conjugateGradientMillisecondsPerIteration << '\n';
	output << "conjugate_gradient_finite_state=" << (summary.conjugateGradientFiniteState ? "true" : "false") << '\n';
	output << "conjugate_gradient_within_reference_budget=" << (summary.conjugateGradientWithinReferenceBudget ? "true" : "false") << '\n';
	output << "conjugate_gradient_probe_passed=" << (summary.conjugateGradientPassed ? "true" : "false") << '\n';
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
	output << "candidate_disposition=continue_coupled_hybrid_pressure_evaluation\n";
	return summary.passed;
}

} // namespace omni::atmospherebench
