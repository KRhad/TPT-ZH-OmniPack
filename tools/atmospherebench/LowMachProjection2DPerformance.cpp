#include "LowMachProjection2DPerformance.h"

#include <algorithm>
#include <array>
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
	constexpr double DivergenceReductionTarget = 0.02;
	constexpr std::size_t PerformanceWarmupCount = 1;
	constexpr std::size_t PerformanceRepeatCount = 5;
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
		result.workingBytesPerCell = 7.0 * sizeof(double);
		result.workingBytesTotal = CellsX * CellsY * 7 * sizeof(double);
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

	struct MultigridLevel
	{
		std::size_t cellsX = 0;
		std::size_t cellsY = 0;
		double cellLengthX = 1.0;
		double cellLengthY = 1.0;
		std::vector<double> rightHandSide;
		std::vector<double> solution;
		std::vector<double> residual;
		std::vector<double> scratch;
	};

	std::size_t MultigridIndex(std::size_t x, std::size_t y, std::size_t cellsX)
	{
		return y * cellsX + x;
	}

	std::size_t WrapIndex(std::ptrdiff_t value, std::size_t size)
	{
		const auto signedSize = static_cast<std::ptrdiff_t>(size);
		value %= signedSize;
		if (value < 0)
			value += signedSize;
		return static_cast<std::size_t>(value);
	}

	void RemoveMean(std::vector<double> &values)
	{
		double sum = 0.0;
		for (const double value : values)
			sum += value;
		const double mean = sum / static_cast<double>(values.size());
		for (double &value : values)
			value -= mean;
	}

	void ApplyMultigridOperator(const MultigridLevel &level,
		const std::vector<double> &input, std::vector<double> &output)
	{
		const double inverseCellLengthXSquared = 1.0
			/ (level.cellLengthX * level.cellLengthX);
		const double inverseCellLengthYSquared = 1.0
			/ (level.cellLengthY * level.cellLengthY);
		for (std::size_t y = 0; y < level.cellsY; ++y)
			for (std::size_t x = 0; x < level.cellsX; ++x)
			{
				const auto index = MultigridIndex(x, y, level.cellsX);
				const auto left = MultigridIndex((x + level.cellsX - 1) % level.cellsX,
					y, level.cellsX);
				const auto right = MultigridIndex((x + 1) % level.cellsX, y, level.cellsX);
				const auto bottom = MultigridIndex(x,
					(y + level.cellsY - 1) % level.cellsY, level.cellsX);
				const auto top = MultigridIndex(x, (y + 1) % level.cellsY, level.cellsX);
				output[index] = (2.0 * input[index] - input[left] - input[right])
					* inverseCellLengthXSquared
					+ (2.0 * input[index] - input[bottom] - input[top])
					* inverseCellLengthYSquared;
			}
	}

	void Smooth(MultigridLevel &level, std::size_t sweeps)
	{
		constexpr double Weight = 2.0 / 3.0;
		const double inverseCellLengthXSquared = 1.0
			/ (level.cellLengthX * level.cellLengthX);
		const double inverseCellLengthYSquared = 1.0
			/ (level.cellLengthY * level.cellLengthY);
		const double inverseDiagonal = 1.0 / (2.0 * inverseCellLengthXSquared
			+ 2.0 * inverseCellLengthYSquared);
		for (std::size_t sweep = 0; sweep < sweeps; ++sweep)
		{
			for (std::size_t y = 0; y < level.cellsY; ++y)
				for (std::size_t x = 0; x < level.cellsX; ++x)
				{
					const auto index = MultigridIndex(x, y, level.cellsX);
					const auto left = MultigridIndex(
						(x + level.cellsX - 1) % level.cellsX, y, level.cellsX);
					const auto right = MultigridIndex(
						(x + 1) % level.cellsX, y, level.cellsX);
					const auto bottom = MultigridIndex(x,
						(y + level.cellsY - 1) % level.cellsY, level.cellsX);
					const auto top = MultigridIndex(x,
						(y + 1) % level.cellsY, level.cellsX);
					const double operatorValue =
						(2.0 * level.solution[index] - level.solution[left]
							- level.solution[right]) * inverseCellLengthXSquared
						+ (2.0 * level.solution[index] - level.solution[bottom]
							- level.solution[top]) * inverseCellLengthYSquared;
					level.scratch[index] = level.solution[index]
						+ Weight * inverseDiagonal
							* (level.rightHandSide[index] - operatorValue);
				}
			level.solution.swap(level.scratch);
			RemoveMean(level.solution);
		}
	}

	void ComputeResidual(MultigridLevel &level)
	{
		ApplyMultigridOperator(level, level.solution, level.scratch);
		for (std::size_t index = 0; index < level.solution.size(); ++index)
			level.residual[index] = level.rightHandSide[index] - level.scratch[index];
		RemoveMean(level.residual);
	}

	void RestrictResidual(const MultigridLevel &fine, MultigridLevel &coarse)
	{
		const std::size_t factorX = fine.cellsX / coarse.cellsX;
		const std::size_t factorY = fine.cellsY / coarse.cellsY;
		const double inverseSampleCount = 1.0
			/ static_cast<double>(factorX * factorY);
		for (std::size_t y = 0; y < coarse.cellsY; ++y)
			for (std::size_t x = 0; x < coarse.cellsX; ++x)
			{
				const auto index = MultigridIndex(x, y, coarse.cellsX);
				double sum = 0.0;
				for (std::size_t offsetY = 0; offsetY < factorY; ++offsetY)
					for (std::size_t offsetX = 0; offsetX < factorX; ++offsetX)
						sum += fine.residual[MultigridIndex(
							x * factorX + offsetX, y * factorY + offsetY,
							fine.cellsX)];
				coarse.rightHandSide[index] = sum * inverseSampleCount;
			}
		RemoveMean(coarse.rightHandSide);
		std::fill(coarse.solution.begin(), coarse.solution.end(), 0.0);
	}

	void ProlongateAndAdd(const MultigridLevel &coarse, MultigridLevel &fine)
	{
		const std::size_t factorX = fine.cellsX / coarse.cellsX;
		const std::size_t factorY = fine.cellsY / coarse.cellsY;
		for (std::size_t y = 0; y < fine.cellsY; ++y)
			for (std::size_t x = 0; x < fine.cellsX; ++x)
			{
				const double coarsePositionX = (static_cast<double>(x) + 0.5)
					/ static_cast<double>(factorX) - 0.5;
				const double coarsePositionY = (static_cast<double>(y) + 0.5)
					/ static_cast<double>(factorY) - 0.5;
				const auto coarseFloorX = static_cast<std::ptrdiff_t>(
					std::floor(coarsePositionX));
				const auto coarseFloorY = static_cast<std::ptrdiff_t>(
					std::floor(coarsePositionY));
				const std::size_t coarseX0 = WrapIndex(coarseFloorX, coarse.cellsX);
				const std::size_t coarseY0 = WrapIndex(coarseFloorY, coarse.cellsY);
				const std::size_t coarseX1 = (coarseX0 + 1) % coarse.cellsX;
				const std::size_t coarseY1 = (coarseY0 + 1) % coarse.cellsY;
				const double fractionX = coarsePositionX
					- static_cast<double>(coarseFloorX);
				const double fractionY = coarsePositionY
					- static_cast<double>(coarseFloorY);
				const double bottom = (1.0 - fractionX)
					* coarse.solution[MultigridIndex(coarseX0, coarseY0, coarse.cellsX)]
					+ fractionX
					* coarse.solution[MultigridIndex(coarseX1, coarseY0, coarse.cellsX)];
				const double top = (1.0 - fractionX)
					* coarse.solution[MultigridIndex(coarseX0, coarseY1, coarse.cellsX)]
					+ fractionX
					* coarse.solution[MultigridIndex(coarseX1, coarseY1, coarse.cellsX)];
				fine.solution[MultigridIndex(x, y, fine.cellsX)] +=
					(1.0 - fractionY) * bottom + fractionY * top;
			}
		RemoveMean(fine.solution);
	}

	std::size_t SelectCoarseningFactor(std::size_t cells)
	{
		if (cells > 4 && cells % 2 == 0)
			return 2;
		if (cells > 4 && cells % 3 == 0)
			return 3;
		return 1;
	}

	void MultigridVCycle(std::vector<MultigridLevel> &levels, std::size_t levelIndex)
	{
		auto &level = levels[levelIndex];
		if (levelIndex + 1 == levels.size())
		{
			Smooth(level, 80);
			return;
		}
		Smooth(level, 4);
		ComputeResidual(level);
		auto &coarse = levels[levelIndex + 1];
		RestrictResidual(level, coarse);
		MultigridVCycle(levels, levelIndex + 1);
		ProlongateAndAdd(coarse, level);
		Smooth(level, 4);
	}

	double ResidualRms(MultigridLevel &level)
	{
		ComputeResidual(level);
		double sum = 0.0;
		for (const double value : level.residual)
			sum += value * value;
		return std::sqrt(sum / static_cast<double>(level.residual.size()));
	}

	template <std::size_t CellsX, std::size_t CellsY>
	LowMachProjection2DPerformanceSample RunMultigridSample()
	{
		LowMachProjection2DPerformanceSample result;
		result.cellsX = CellsX;
		result.cellsY = CellsY;
		std::vector<double> velocityX(CellsX * CellsY);
		std::vector<double> velocityY(CellsX * CellsY);
		std::vector<double> divergence(CellsX * CellsY);
		InitializeVelocity<CellsX, CellsY>(velocityX, velocityY);
		result.initialDivergenceL2 = Divergence<CellsX, CellsY>(velocityX, velocityY,
			divergence);
		std::vector<MultigridLevel> levels;
		std::size_t cellsX = CellsX;
		std::size_t cellsY = CellsY;
		double cellLengthX = CellLength;
		double cellLengthY = CellLength;
		while (true)
		{
			MultigridLevel level;
			level.cellsX = cellsX;
			level.cellsY = cellsY;
			level.cellLengthX = cellLengthX;
			level.cellLengthY = cellLengthY;
			const std::size_t count = cellsX * cellsY;
			level.rightHandSide.resize(count);
			level.solution.assign(count, 0.0);
			level.residual.resize(count);
			level.scratch.resize(count);
			levels.push_back(std::move(level));
			const std::size_t factorX = SelectCoarseningFactor(cellsX);
			const std::size_t factorY = SelectCoarseningFactor(cellsY);
			if (factorX == 1 && factorY == 1)
				break;
			cellsX /= factorX;
			cellsY /= factorY;
			cellLengthX *= static_cast<double>(factorX);
			cellLengthY *= static_cast<double>(factorY);
		}
		for (std::size_t index = 0; index < divergence.size(); ++index)
			levels.front().rightHandSide[index] = -divergence[index] / TimeStep;
		result.workingBytesTotal = CellsX * CellsY * 3 * sizeof(double);
		for (const auto &level : levels)
			result.workingBytesTotal += level.solution.size() * 4 * sizeof(double);
		result.workingBytesPerCell = static_cast<double>(result.workingBytesTotal)
			/ static_cast<double>(CellsX * CellsY);
		RemoveMean(levels.front().rightHandSide);
		const auto start = std::chrono::steady_clock::now();
		for (std::size_t cycle = 0; cycle < 30; ++cycle)
		{
			MultigridVCycle(levels, 0);
			result.iterationCount = cycle + 1;
			const double projectedDivergenceRatio = ResidualRms(levels.front())
				* TimeStep / result.initialDivergenceL2;
			if (projectedDivergenceRatio < DivergenceReductionTarget)
				break;
		}
		ApplyCorrection<CellsX, CellsY>(velocityX, velocityY, levels.front().solution);
		const auto end = std::chrono::steady_clock::now();
		result.elapsedMilliseconds = std::chrono::duration<double, std::milli>(end - start).count();
		result.finalDivergenceL2 = Divergence<CellsX, CellsY>(velocityX, velocityY,
			divergence);
		result.divergenceReductionRatio = result.finalDivergenceL2
			/ result.initialDivergenceL2;
		result.finiteState = result.iterationCount > 0
			&& std::isfinite(result.elapsedMilliseconds)
			&& std::isfinite(result.finalDivergenceL2)
			&& std::isfinite(result.divergenceReductionRatio);
		result.divergenceReduced = result.divergenceReductionRatio
			< DivergenceReductionTarget;
		result.withinReferenceBudget = result.elapsedMilliseconds <= ReferenceBudgetMilliseconds;
		result.passed = result.finiteState;
		return result;
	}

	template <std::size_t CellsX, std::size_t CellsY>
	LowMachProjection2DPerformanceSample RunMultigridMedianSample()
	{
		for (std::size_t warmup = 0; warmup < PerformanceWarmupCount; ++warmup)
			(void)RunMultigridSample<CellsX, CellsY>();
		std::array<LowMachProjection2DPerformanceSample, PerformanceRepeatCount> samples;
		for (auto &sample : samples)
			sample = RunMultigridSample<CellsX, CellsY>();
		std::sort(samples.begin(), samples.end(), [](const auto &left, const auto &right) {
			return left.elapsedMilliseconds < right.elapsedMilliseconds;
		});
		return samples[PerformanceRepeatCount / 2];
	}
}

LowMachProjection2DPerformanceSummary RunLowMachProjection2DPerformance()
{
	LowMachProjection2DPerformanceSummary summary;
	summary.referenceAtmosphereBudgetMilliseconds = ReferenceBudgetMilliseconds;
	summary.atmosphereGrid = RunSample<153, 96>();
	summary.doubledGrid = RunSample<306, 192>();
	summary.particleGrid = RunSample<612, 384>();
	summary.multigridAtmosphereGrid = RunMultigridMedianSample<153, 96>();
	summary.multigridDoubledGrid = RunMultigridMedianSample<306, 192>();
	summary.multigridParticleGrid = RunMultigridMedianSample<612, 384>();
	summary.targetGridMatrixMeasured = summary.atmosphereGrid.finiteState
		&& summary.doubledGrid.finiteState && summary.particleGrid.finiteState;
	summary.atmosphereGridWithinBudget = summary.atmosphereGrid.withinReferenceBudget;
	summary.multigridTargetGridMatrixMeasured = summary.multigridAtmosphereGrid.finiteState
		&& summary.multigridDoubledGrid.finiteState
		&& summary.multigridParticleGrid.finiteState;
	summary.multigridAtmosphereGridWithinBudget =
		summary.multigridAtmosphereGrid.withinReferenceBudget;
	summary.passed = summary.targetGridMatrixMeasured
		&& summary.multigridTargetGridMatrixMeasured;
	return summary;
}

bool WriteLowMachProjection2DPerformance(std::ostream &output)
{
	const auto summary = RunLowMachProjection2DPerformance();
	output << "schema_version=1\n";
	output << "case=low_mach_pressure_projection_2d_target_matrix\n";
	output << "candidate=periodic_low_mach_cg_multigrid_components\n";
	output << "candidate_solver_implemented=false\n";
	output << "atmosphere_solver_selection=unselected\n";
	output << "physical_scale_selection=unselected\n";
	output << "physical_time_policy=unselected\n";
	output << "result_status=low_mach_target_matrix_component_comparison\n";
	output << "case_time_domain=nondimensional_contract\n";
	output << "case_timestep=" << TimeStep << '\n';
	output << "case_step_count=1\n";
	output << "boundary_mode=periodic\n";
	output << "grid_cells_x=153\n";
	output << "grid_cells_y=96\n";
	output << "grid_cell_count=14688\n";
	output << "cell_length=1\n";
	output << "state_bytes_per_cell=24\n";
	output << "state_and_flux_scratch_bytes_per_cell="
		<< summary.atmosphereGrid.workingBytesPerCell << '\n';
	output << "state_and_flux_scratch_bytes_total="
		<< summary.atmosphereGrid.workingBytesTotal << '\n';
	output << "reference_atmosphere_budget_milliseconds=" << summary.referenceAtmosphereBudgetMilliseconds << '\n';
	output << "multigrid_performance_warmup_count=" << PerformanceWarmupCount << '\n';
	output << "multigrid_performance_repeat_count=" << PerformanceRepeatCount << '\n';
	output << "target_grid_matrix_measured=" << (summary.targetGridMatrixMeasured ? "true" : "false") << '\n';
	output << "atmosphere_grid_within_budget=" << (summary.atmosphereGridWithinBudget ? "true" : "false") << '\n';
	output << "target_matrix_convergence_passed="
		<< (summary.atmosphereGrid.divergenceReduced && summary.doubledGrid.divergenceReduced
			&& summary.particleGrid.divergenceReduced ? "true" : "false") << '\n';
	output << "target_matrix_budget_passed="
		<< (summary.atmosphereGridWithinBudget ? "true" : "false") << '\n';
	output << "multigrid_target_grid_matrix_measured="
		<< (summary.multigridTargetGridMatrixMeasured ? "true" : "false") << '\n';
	output << "multigrid_target_matrix_convergence_passed="
		<< (summary.multigridAtmosphereGrid.divergenceReduced
			&& summary.multigridDoubledGrid.divergenceReduced
			&& summary.multigridParticleGrid.divergenceReduced ? "true" : "false") << '\n';
	output << "multigrid_atmosphere_grid_within_budget="
		<< (summary.multigridAtmosphereGridWithinBudget ? "true" : "false") << '\n';
	output << "multigrid_target_matrix_budget_passed="
		<< (summary.multigridAtmosphereGrid.withinReferenceBudget
			&& summary.multigridDoubledGrid.withinReferenceBudget
			&& summary.multigridParticleGrid.withinReferenceBudget ? "true" : "false") << '\n';
	output << "multigrid_reference_grid_budget_passed="
		<< (summary.multigridAtmosphereGridWithinBudget ? "true" : "false") << '\n';
	output << "selected_low_mach_component=geometric_multigrid_v_cycle\n";
	output << "low_mach_component_selection_ready="
		<< (summary.multigridTargetGridMatrixMeasured
			&& summary.multigridAtmosphereGrid.divergenceReduced
			&& summary.multigridDoubledGrid.divergenceReduced
			&& summary.multigridParticleGrid.divergenceReduced
			&& summary.multigridAtmosphereGridWithinBudget ? "true" : "false") << '\n';
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
		output << prefix << "_working_bytes_per_cell=" << sample.workingBytesPerCell << '\n';
		output << prefix << "_working_bytes_total=" << sample.workingBytesTotal << '\n';
		output << prefix << "_finite_state=" << (sample.finiteState ? "true" : "false") << '\n';
		output << prefix << "_divergence_reduced=" << (sample.divergenceReduced ? "true" : "false") << '\n';
		output << prefix << "_within_reference_budget=" << (sample.withinReferenceBudget ? "true" : "false") << '\n';
		output << prefix << "_passed=" << (sample.passed ? "true" : "false") << '\n';
	}
	for (const auto &sample : {summary.multigridAtmosphereGrid,
		summary.multigridDoubledGrid, summary.multigridParticleGrid})
	{
		const char *prefix = sample.cellsX == 153 ? "multigrid_atmosphere_grid" :
			sample.cellsX == 306 ? "multigrid_doubled_grid" : "multigrid_particle_grid";
		output << prefix << "_cells_x=" << sample.cellsX << '\n';
		output << prefix << "_cells_y=" << sample.cellsY << '\n';
		output << prefix << "_cell_count=" << sample.cellsX * sample.cellsY << '\n';
		output << prefix << "_v_cycle_count=" << sample.iterationCount << '\n';
		output << prefix << "_initial_divergence_l2=" << sample.initialDivergenceL2 << '\n';
		output << prefix << "_final_divergence_l2=" << sample.finalDivergenceL2 << '\n';
		output << prefix << "_divergence_reduction_ratio=" << sample.divergenceReductionRatio << '\n';
		output << prefix << "_elapsed_milliseconds=" << sample.elapsedMilliseconds << '\n';
		output << prefix << "_working_bytes_per_cell=" << sample.workingBytesPerCell << '\n';
		output << prefix << "_working_bytes_total=" << sample.workingBytesTotal << '\n';
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
	output << "candidate_disposition=select_geometric_multigrid_component_for_hybrid_coupling_evaluation\n";
	return summary.passed;
}

} // namespace omni::atmospherebench
