#pragma once

#include "AtmosphereBench.h"
#include "Rusanov1D.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct HybridTransportProbeSummary
{
	BenchmarkCase benchmarkCase;
	ConservationLedger ledger;
	NumericalCorrectionLedger corrections{};
	double referenceVelocity = 0.0;
	double simulatedTime = 0.0;
	double maximumAdvectiveCfl = 0.0;
	double minimumDensity = 0.0;
	double maximumDensity = 0.0;
	double minimumPressure = 0.0;
	double densityL1Error = 0.0;
	double densityLinfError = 0.0;
	double pressureLinfError = 0.0;
	double totalVariationRatio = 0.0;
	std::size_t referenceShiftCells = 0;
	std::size_t stateAndScratchBytesTotal = 0;
	bool positivityPreserved = false;
	bool densityBoundsPreserved = false;
	bool advectionReferencePassed = false;
	bool passed = false;
};

struct HybridPolicyProbeSummary
{
	HybridTransportProbeSummary moderateMach;
	HybridTransportProbeSummary lowMach;
	HybridTransportProbeSummary veryLowMach;
	RusanovProbeSummary compressibleSod;
	double moderateNominalMach = 0.0;
	double lowNominalMach = 0.0;
	double veryLowNominalMach = 0.0;
	double lowToModerateL1Ratio = 0.0;
	double veryLowToModerateL1Ratio = 0.0;
	std::size_t lowMachRouteCount = 0;
	std::size_t compressibleRouteCount = 0;
	bool lowMachSuitabilityPassed = false;
	bool crossRouteBoundaryCouplingImplemented = false;
	bool eventLocalSubcyclingImplemented = false;
	bool policySelectionReady = false;
	bool passed = false;
};

HybridPolicyProbeSummary RunHybridAllSpeedPolicyProbe();
bool WriteHybridAllSpeedPolicyProbe(std::ostream &output);

} // namespace omni::atmospherebench
