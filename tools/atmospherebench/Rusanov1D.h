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
	double initialMaximumPressure = 0.0;
	double finalMaximumPressure = 0.0;
	double stateChangeL1 = 0.0;
	bool positivityPreserved = false;
	bool stateEvolved = false;
	bool pressurePeakReduced = false;
	bool passed = false;
};

RusanovProbeSummary RunRusanovUniform();
RusanovProbeSummary RunRusanovPressurePulse();
bool WriteRusanovUniformProbe(std::ostream &output);
bool WriteRusanovPressurePulseProbe(std::ostream &output);

} // namespace omni::atmospherebench
