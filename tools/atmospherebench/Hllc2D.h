#pragma once

#include "AtmosphereBench.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct Hllc2DProbeSummary
{
	BenchmarkCase benchmarkCase;
	ConservationLedger ledger;
	NumericalCorrectionLedger corrections{};
	double maximumCfl = 0.0;
	double minimumDensity = 0.0;
	double minimumPressure = 0.0;
	double initialMaximumPressure = 0.0;
	double finalMaximumPressure = 0.0;
	double stateChangeL1 = 0.0;
	std::size_t fluxFallbackCount = 0;
	bool positivityPreserved = false;
	bool stateEvolved = false;
	bool pressurePeakReduced = false;
	bool passed = false;
};

Hllc2DProbeSummary RunHllc2DUniform();
Hllc2DProbeSummary RunHllc2DPressurePulse();
bool WriteHllc2DUniformProbe(std::ostream &output);
bool WriteHllc2DPressurePulseProbe(std::ostream &output);

} // namespace omni::atmospherebench
