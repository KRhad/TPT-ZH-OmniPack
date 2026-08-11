#pragma once

#include "AtmosphereBench.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct LegacyLikeProbeSummary
{
	BenchmarkCase benchmarkCase;
	NumericalCorrectionLedger corrections{};
	double initialPressureSum = 0.0;
	double finalPressureSum = 0.0;
	double pressureSumDrift = 0.0;
	double initialVelocityXSum = 0.0;
	double finalVelocityXSum = 0.0;
	double initialVelocityYSum = 0.0;
	double finalVelocityYSum = 0.0;
	double initialMaximumPressure = 0.0;
	double finalMaximumPressure = 0.0;
	double minimumPressure = 0.0;
	double maximumAbsoluteVelocity = 0.0;
	double stateChangeL1 = 0.0;
	double stateAndScratchBytesPerCell = 0.0;
	std::size_t stateAndScratchBytesTotal = 0;
	bool finiteState = false;
	bool uniformPreserved = false;
	bool stateEvolved = false;
	bool pressurePeakReduced = false;
	bool pressureSumPreserved = false;
	bool passed = false;
};

LegacyLikeProbeSummary RunLegacyLikeUniform();
LegacyLikeProbeSummary RunLegacyLikePressurePulse();
bool WriteLegacyLikeUniformProbe(std::ostream &output);
bool WriteLegacyLikePressurePulseProbe(std::ostream &output);

} // namespace omni::atmospherebench
