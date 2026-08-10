#pragma once

#include "AtmosphereBench.h"

#include <iosfwd>

namespace omni::atmospherebench
{

struct RusanovProbeSummary
{
	BenchmarkCase benchmarkCase;
	ConservationLedger ledger;
	NumericalCorrectionLedger corrections{};
	double maximumCfl = 0.0;
	double minimumDensity = 0.0;
	double minimumPressure = 0.0;
	double minimumEnergyDensity = 0.0;
	bool positivityPreserved = false;
	bool passed = false;
};

RusanovProbeSummary RunRusanovUniform();
bool WriteRusanovUniformProbe(std::ostream &output);

} // namespace omni::atmospherebench
