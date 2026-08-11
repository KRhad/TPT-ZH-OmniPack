#pragma once

#include "AtmosphereBench.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct HybridMixedRegion2DGridSample
{
	std::size_t cellsX = 0;
	std::size_t cellsY = 0;
	std::size_t cellCount = 0;
	std::size_t macroSteps = 0;
	std::size_t maximumEventCells = 0;
	double maximumEventFraction = 0.0;
	double elapsedMilliseconds = 0.0;
	double millisecondsPerMacroStep = 0.0;
	std::size_t workingBytesTotal = 0;
	bool promotionObserved = false;
	bool crossRouteFaceObserved = false;
	bool refluxConservationPassed = false;
	bool globalLedgerCloses = false;
	bool positivityPreserved = false;
	bool valid = false;
};

struct HybridMixedRegion2DProbeSummary
{
	BenchmarkCase benchmarkCase;
	ConservationLedger ledger;
	ConservativeState interfaceEventExchange{};
	ConservativeState interfaceBulkExchange{};
	NumericalCorrectionLedger corrections{};
	std::size_t macroStepCount = 0;
	std::size_t eventSubstepsPerMacro = 0;
	std::size_t promotionCount = 0;
	std::size_t demotionCount = 0;
	std::size_t crossRouteFaceCount = 0;
	std::size_t maximumEventCells = 0;
	std::size_t maximumHaloCells = 0;
	std::size_t maximumEventSubstepsUsed = 0;
	std::size_t hllcFallbackCount = 0;
	double maximumCfl = 0.0;
	double maximumEventFraction = 0.0;
	double minimumDensity = 0.0;
	double minimumPressure = 0.0;
	double initialPressureJump = 0.0;
	double finalPressureJump = 0.0;
	double physicalAcousticDomainCells = 0.0;
	HybridMixedRegion2DGridSample legacyGrid;
	HybridMixedRegion2DGridSample doubledGrid;
	HybridMixedRegion2DGridSample particleGrid;
	bool promotionPassed = false;
	bool demotionPassed = false;
	bool crossRouteFacePassed = false;
	bool refluxConservationPassed = false;
	bool interfaceLedgerCloses = false;
	bool hysteresisConflictPassed = false;
	bool thresholdScanPassed = false;
	bool dynamicEventRegionImplemented = false;
	bool eventLocalSubcyclingImplemented = false;
	bool positivityPreserved = false;
	bool finiteState = false;
	bool globalLedgerCloses = false;
	bool physicalDomainExceedsBenchmark = false;
	bool targetGridMatrixMeasured = false;
	bool passed = false;
};

HybridMixedRegion2DProbeSummary RunHybridMixedRegion2DProbe();
bool WriteHybridMixedRegion2DProbe(std::ostream &output);

} // namespace omni::atmospherebench
