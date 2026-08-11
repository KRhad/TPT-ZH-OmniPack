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
	ConservativeSourceLedger boundary{};
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
	bool sourceAndBoundaryLedgerCloses = false;
	bool passed = false;
};

struct Hllc2DNaturalConvectionSummary
{
	BenchmarkCase benchmarkCase;
	Hllc2DProbeSummary control;
	Hllc2DProbeSummary heated;
	double gravityY = 0.0;
	double hotTemperatureAmplitude = 0.0;
	double initialThermalCenterY = 0.0;
	double finalThermalCenterY = 0.0;
	double thermalCenterRise = 0.0;
	double thermalWeightedVelocityY = 0.0;
	double controlMaximumAbsoluteVelocity = 0.0;
	double heatedMaximumUpwardVelocity = 0.0;
	double heatedMinimumDownwardVelocity = 0.0;
	double heatedMaximumAbsoluteVelocity = 0.0;
	double maximumUpwardVelocityDifference = 0.0;
	double minimumDownwardVelocityDifference = 0.0;
	double maximumAbsoluteVelocityDifference = 0.0;
	bool circulationObserved = false;
	bool passed = false;
};

Hllc2DProbeSummary RunHllc2DUniform();
Hllc2DProbeSummary RunHllc2DPressurePulse();
Hllc2DProbeSummary RunHllc2DSealedHeating();
Hllc2DNaturalConvectionSummary RunHllc2DNaturalConvection();
bool WriteHllc2DUniformProbe(std::ostream &output);
bool WriteHllc2DPressurePulseProbe(std::ostream &output);
bool WriteHllc2DSealedHeatingProbe(std::ostream &output);
bool WriteHllc2DNaturalConvectionProbe(std::ostream &output);

} // namespace omni::atmospherebench
