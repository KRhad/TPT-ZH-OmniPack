#pragma once

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct LowMachProjection2DPerformanceSample
{
	std::size_t cellsX = 0;
	std::size_t cellsY = 0;
	std::size_t iterationCount = 0;
	double initialDivergenceL2 = 0.0;
	double finalDivergenceL2 = 0.0;
	double divergenceReductionRatio = 0.0;
	double elapsedMilliseconds = 0.0;
	double workingBytesPerCell = 0.0;
	std::size_t workingBytesTotal = 0;
	bool finiteState = false;
	bool divergenceReduced = false;
	bool withinReferenceBudget = false;
	bool passed = false;
};

struct LowMachProjection2DPerformanceSummary
{
	LowMachProjection2DPerformanceSample atmosphereGrid;
	LowMachProjection2DPerformanceSample doubledGrid;
	LowMachProjection2DPerformanceSample particleGrid;
	LowMachProjection2DPerformanceSample multigridAtmosphereGrid;
	LowMachProjection2DPerformanceSample multigridDoubledGrid;
	LowMachProjection2DPerformanceSample multigridParticleGrid;
	double referenceAtmosphereBudgetMilliseconds = 0.0;
	bool targetGridMatrixMeasured = false;
	bool atmosphereGridWithinBudget = false;
	bool multigridTargetGridMatrixMeasured = false;
	bool multigridAtmosphereGridWithinBudget = false;
	bool passed = false;
};

LowMachProjection2DPerformanceSummary RunLowMachProjection2DPerformance();
bool WriteLowMachProjection2DPerformance(std::ostream &output);

} // namespace omni::atmospherebench
