#pragma once

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct LowMachProjection2DSummary
{
	std::size_t cellsX = 0;
	std::size_t cellsY = 0;
	std::size_t iterationCount = 0;
	double timeStep = 0.0;
	double initialDivergenceL2 = 0.0;
	double finalDivergenceL2 = 0.0;
	double divergenceReductionRatio = 0.0;
	double initialKineticEnergy = 0.0;
	double finalKineticEnergy = 0.0;
	double pressureCorrectionMean = 0.0;
	double pressureCorrectionMaximumAbsolute = 0.0;
	double elapsedMilliseconds = 0.0;
	double millisecondsPerIteration = 0.0;
	std::size_t workingBytesPerCell = 0;
	std::size_t workingBytesTotal = 0;
	bool finiteState = false;
	bool divergenceReduced = false;
	bool soundSpeedIndependent = false;
	bool passed = false;
};

LowMachProjection2DSummary RunLowMachProjection2D();
bool WriteLowMachProjection2D(std::ostream &output);

} // namespace omni::atmospherebench
