#include "LowMachProjection2DPerformance.h"

#include <chrono>
#include <cmath>
#include <vector>

namespace omni::atmospherebench
{

namespace
{
	constexpr double CellLength = 1.0;
	constexpr double TimeStep = 0.1;
	constexpr std::size_t MaximumIterations = 500;
	constexpr double Tolerance = 1e-10;
	constexpr double ReferenceBudgetMilliseconds = 1000.0 / 60.0 / 4.0;
	constexpr double Pi = 3.141592653589793238462643383279502884;

	template <std::size_t CellsX, std::size_t CellsY>
	std::size_t Index(std::size_t x, std::size_t y)
	{
		return y * CellsX + x;
	}

	template <std::size_t CellsX, std::size_t CellsY>
	void InitializeVelocity(std::vector<double> &velocityX,
		std::vector<double> &velocityY)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const double sampleX = static_cast<double>(x) + 0.5;
				const double sampleY = static_cast<double>(y) + 0.5;
				velocityX[Index<CellsX, CellsY>(x, y)] =
					0.080 * std::sin(2.0 * Pi * sampleX / CellsX)
					+ 0.023 * std::sin(6.0 * Pi * sampleX / CellsX)
					+ 0.017 * std::cos(10.0 * Pi * sampleX / CellsX)
					+ 0.011 * std::sin(2.0 * Pi * (sampleX + sampleY) / CellsX);
				velocityY[Index<CellsX, CellsY>(x, y)] =
					0.050 * std::sin(2.0 * Pi * sampleY / CellsY)
					+ 0.019 * std::sin(8.0 * Pi * sampleY / CellsY)
					+ 0.013 * std::cos(12.0 * Pi * sampleY / CellsY)
					+ 0.009 * std::cos(2.0 * Pi * (sampleX - sampleY) / CellsY);
			}
	}

	template <std::size_t CellsX, std::size_t CellsY>
	double Divergence(const std::vector<double> &velocityX,
		const std::vector<double> &velocityY, std::vector<double> &output)
	{
		double sum = 0.0;
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index<CellsX, CellsY>(x, y);
				const auto left = Index<CellsX, CellsY>((x + CellsX - 1) % CellsX, y);
				const auto bottom = Index<CellsX, CellsY>(x, (y + CellsY - 1) % CellsY);
				const double value = (velocityX[index] - velocityX[left]
					+ velocityY[index] - velocityY[bottom]) / CellLength;
				output[index] = value;
				sum += value * value;
			}
		return std::sqrt(sum / static_cast<double>(CellsX * CellsY));
	}

	template <std::size_t CellsX, std::size_t CellsY>
	void ApplyOperator(const std::vector<double> &input, std::vector<double> &output)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index<CellsX, CellsY>(x, y);
				const auto left = Index<CellsX, CellsY>((x + CellsX - 1) % CellsX, y);
				const auto right = Index<CellsX, CellsY>((x + 1) % CellsX, y);
				const auto bottom = Index<CellsX, CellsY>(x, (y + CellsY - 1) % CellsY);
				const auto top = Index<CellsX, CellsY>(x, (y + 1) % CellsY);
				output[index] = (4.0 * input[index] - input[left] - input[right]
					- input[bottom] - input[top]) / (CellLength * CellLength);
			}
	}

	template <std::size_t CellsX, std::size_t CellsY>
	void ApplyCorrection(std::vector<double> &velocityX, std::vector<double> &velocityY,
		const std::vector<double> &pressure)
	{
		for (std::size_t y = 0; y < CellsY; ++y)
			for (std::size_t x = 0; x < CellsX; ++x)
			{
				const auto index = Index<CellsX, CellsY>(x, y);
				const auto right = Index<CellsX, CellsY>((x + 1) % CellsX, y);
				const auto top = Index<CellsX, CellsY>(x, (y + 1) % CellsY);
				velocityX[index] -= TimeStep * (pressure[right] - pressure[index]) / CellLength;
				velocityY[index] -= TimeStep * (pressure[top] - pressure[index]) / CellLength;
			}
	}

	template <std::size_t CellsX, std::size_t CellsY>
LowMachProjection2DPerformanceSample RunSample()
{
		LowMachProjection2DPerformanceSample result;
		result.cellsX = CellsX;
		result.cellsY = CellsY;
		std::vector<double> velocityX(CellsX * CellsY);
		std::vector<double> velocityY(CellsX * CellsY);
		std::vector<double> divergence(CellsX * CellsY);
		std::vector<double> pressure(CellsX * CellsY, 0.0);
		std::vector<double> residual(CellsX * CellsY);
		std::vector<double> direction(CellsX * CellsY);
		std::vector<double> operatorValue(CellsX * CellsY);
		InitializeVelocity<CellsX, CellsY>(velocityX, velocityY);
		result.initialDivergenceL2 = Divergence<CellsX, CellsY>(velocityX, velocityY, divergence);
		for (std::size_t index = 0; index < pressure.size(); ++index)
		{
			residual[index] = -divergence[index] / TimeStep;
			direction[index] = residual[index];
		}
		double residualSquared = 0.0;
		for (const double value : residual)
			residualSquared += value * value;
		const auto start = std::chrono::steady_clock::now();
		for (std::size_t iteration = 0; iteration < MaximumIterations; ++iteration)
		{
			ApplyOperator<CellsX, CellsY>(direction, operatorValue);
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
			if (std::sqrt(nextResidualSquared / residual.size()) < Tolerance)
				break;
			const double beta = nextResidualSquared / residualSquared;
			for (std::size_t index = 0; index < direction.size(); ++index)
				direction[index] = residual[index] + beta * direction[index];
			residualSquared = nextResidualSquared;
		}
		ApplyCorrection<CellsX, CellsY>(velocityX, velocityY, pressure);
		const auto end = std::chrono::steady_clock::now();
		result.elapsedMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
		result.finalDivergenceL2 = Divergence<CellsX, CellsY>(velocityX, velocityY, divergence);
		result.divergenceReductionRatio = result.finalDivergenceL2 / result.initialDivergenceL2;
		result.finiteState = result.iterationCount > 0
			&& std::isfinite(result.elapsedMilliseconds)
			&& std::isfinite(result.finalDivergenceL2)
			&& std::isfinite(result.divergenceReductionRatio);
		result.divergenceReduced = result.divergenceReductionRatio < 0.02;
		result.withinReferenceBudget = result.elapsedMilliseconds <= ReferenceBudgetMilliseconds;
		result.passed = result.finiteState;
		return result;
	}
}

LowMachProjection2DPerformanceSummary RunLowMachProjection2DPerformance()
{
	LowMachProjection2DPerformanceSummary summary;
	summary.referenceAtmosphereBudgetMilliseconds = ReferenceBudgetMilliseconds;
	summary.atmosphereGrid = RunSample<153, 96>();
	summary.doubledGrid = RunSample<306, 192>();
	summary.particleGrid = RunSample<612, 384>();
	summary.targetGridMatrixMeasured = summary.atmosphereGrid.finiteState
		&& summary.doubledGrid.finiteState && summary.particleGrid.finiteState;
	summary.atmosphereGridWithinBudget = summary.atmosphereGrid.withinReferenceBudget;
	summary.passed = summary.targetGridMatrixMeasured;
	return summary;
}

bool WriteLowMachProjection2DPerformance(std::ostream &output)
{
	const auto summary = RunLowMachProjection2DPerformance();
	output << "schema_version=1\n";
	output << "case=low_mach_pressure_projection_2d_target_matrix\n";
	output << "candidate=periodic_low_mach_conjugate_gradient_component\n";
	output << "candidate_solver_implemented=false\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "result_status=low_mach_target_matrix_component_probe\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "case_timestep=" << TimeStep << '\n';
	output << "case_step_count=1\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=153\n";
	output << "grid_cells_y=96\n";
	output << "grid_cell_count=14688\n";
	output << "cell_length=1\n";
	output << "state_bytes_per_cell=40\n";
	output << "state_and_flux_scratch_bytes_per_cell=40\n";
	output << "state_and_flux_scratch_bytes_total=587520\n";
	output << "reference_atmosphere_budget_milliseconds=" << summary.referenceAtmosphereBudgetMilliseconds << '\n';
	output << "target_grid_matrix_measured=" << (summary.targetGridMatrixMeasured ? "true" : "false") << '\n';
	output << "atmosphere_grid_within_budget=" << (summary.atmosphereGridWithinBudget ? "true" : "false") << '\n';
	output << "target_matrix_convergence_passed="
		<< (summary.atmosphereGrid.divergenceReduced && summary.doubledGrid.divergenceReduced
			&& summary.particleGrid.divergenceReduced ? "true" : "false") << '\n';
	output << "target_matrix_budget_passed="
		<< (summary.atmosphereGridWithinBudget ? "true" : "false") << '\n';
	for (const auto &sample : {summary.atmosphereGrid, summary.doubledGrid, summary.particleGrid})
	{
		const char *prefix = sample.cellsX == 153 ? "atmosphere_grid" :
			sample.cellsX == 306 ? "doubled_grid" : "particle_grid";
		output << prefix << "_cells_x=" << sample.cellsX << '\n';
		output << prefix << "_cells_y=" << sample.cellsY << '\n';
		output << prefix << "_cell_count=" << sample.cellsX * sample.cellsY << '\n';
		output << prefix << "_iteration_count=" << sample.iterationCount << '\n';
		output << prefix << "_initial_divergence_l2=" << sample.initialDivergenceL2 << '\n';
		output << prefix << "_final_divergence_l2=" << sample.finalDivergenceL2 << '\n';
		output << prefix << "_divergence_reduction_ratio=" << sample.divergenceReductionRatio << '\n';
		output << prefix << "_elapsed_milliseconds=" << sample.elapsedMilliseconds << '\n';
		output << prefix << "_finite_state=" << (sample.finiteState ? "true" : "false") << '\n';
		output << prefix << "_divergence_reduced=" << (sample.divergenceReduced ? "true" : "false") << '\n';
		output << prefix << "_within_reference_budget=" << (sample.withinReferenceBudget ? "true" : "false") << '\n';
		output << prefix << "_passed=" << (sample.passed ? "true" : "false") << '\n';
	}
	output << "general_low_mach_pressure_coupling=implemented_periodic_projection_component_only\n";
	output << "compressible_event_coupling=not_implemented_in_this_component\n";
	output << "near_vacuum_support=not_applicable_bulk_component_routes_vacuum_to_compressible\n";
	output << "production_runtime_integration=not_implemented\n";
	output << "solver_selection=unselected\n";
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
	output << "target_matrix_probe_passed=" << (summary.passed ? "true" : "false") << '\n';
	output << "candidate_disposition=reject_unpreconditioned_cg_target_budget_continue_multigrid_or_preconditioner\n";
	return summary.passed;
}

} // namespace omni::atmospherebench
