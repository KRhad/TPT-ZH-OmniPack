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
	ConservativeState boundaryExchange{};
	double maximumCfl = 0.0;
	double minimumDensity = 0.0;
	double maximumDensity = 0.0;
	double minimumPressure = 0.0;
	double minimumEnergyDensity = 0.0;
	double initialMaximumPressure = 0.0;
	double finalMaximumPressure = 0.0;
	double stateChangeL1 = 0.0;
	double densityL1Error = 0.0;
	double densityLinfError = 0.0;
	double pressureLinfError = 0.0;
	double totalVariationRatio = 0.0;
	double referenceVelocity = 0.0;
	double initialLowDensityRegionMass = 0.0;
	double finalLowDensityRegionMass = 0.0;
	double minimumVelocityX = 0.0;
	double maximumVelocityX = 0.0;
	double simulatedTime = 0.0;
	double shockPosition = 0.0;
	std::size_t referenceShiftCells = 0;
	bool positivityPreserved = false;
	bool stateEvolved = false;
	bool pressurePeakReduced = false;
	bool advectionReferencePassed = false;
	bool densityBoundsPreserved = false;
	bool lowDensityRegionMassIncreased = false;
	bool boundaryLedgerCloses = false;
	bool shockReferencePassed = false;
	bool passed = false;
};

RusanovProbeSummary RunRusanovUniform();
RusanovProbeSummary RunRusanovPressurePulse();
RusanovProbeSummary RunRusanovDensityAdvection();
RusanovProbeSummary RunRusanovContactDiscontinuity();
RusanovProbeSummary RunRusanovNearVacuumExpansion();
RusanovProbeSummary RunRusanovSodShockTube();
bool WriteRusanovUniformProbe(std::ostream &output);
bool WriteRusanovPressurePulseProbe(std::ostream &output);
bool WriteRusanovDensityAdvectionProbe(std::ostream &output);
bool WriteRusanovContactDiscontinuityProbe(std::ostream &output);
bool WriteRusanovNearVacuumExpansionProbe(std::ostream &output);
bool WriteRusanovSodShockTubeProbe(std::ostream &output);

} // namespace omni::atmospherebench
