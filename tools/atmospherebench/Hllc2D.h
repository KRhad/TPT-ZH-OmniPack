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
	ConservativeSourceLedger sources{};
	NumericalCorrectionLedger corrections{};
	double maximumCfl = 0.0;
	double minimumDensity = 0.0;
	double minimumPressure = 0.0;
	double initialMaximumPressure = 0.0;
	double finalMaximumPressure = 0.0;
	double initialMeanPressure = 0.0;
	double finalMeanPressure = 0.0;
	double initialMeanTemperature = 0.0;
	double finalMeanTemperature = 0.0;
	double expectedFinalMeanPressure = 0.0;
	double expectedFinalMeanTemperature = 0.0;
	double stateChangeL1 = 0.0;
	double stateAndFluxScratchBytesPerCell = 0.0;
	std::size_t stateAndFluxScratchBytesTotal = 0;
	std::size_t fluxFallbackCount = 0;
	bool positivityPreserved = false;
	bool stateEvolved = false;
	bool pressurePeakReduced = false;
	bool pressureIncreased = false;
	bool temperatureIncreased = false;
	bool sourceLedgerCloses = false;
	bool passed = false;
};

Hllc2DProbeSummary RunHllc2DUniform();
Hllc2DProbeSummary RunHllc2DPressurePulse();
Hllc2DProbeSummary RunHllc2DSealedHeating();
bool WriteHllc2DUniformProbe(std::ostream &output);
bool WriteHllc2DPressurePulseProbe(std::ostream &output);
bool WriteHllc2DSealedHeatingProbe(std::ostream &output);

} // namespace omni::atmospherebench
