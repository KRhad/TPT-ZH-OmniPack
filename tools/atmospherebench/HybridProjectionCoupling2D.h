#pragma once

#include "AtmosphereBench.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct HybridProjectionCoupling2DSummary
{
	ConservationLedger ledger;
	NumericalCorrectionLedger corrections;
	std::size_t cellsX = 0;
	std::size_t cellsY = 0;
	std::size_t projectionIterations = 0;
	std::size_t crossRouteFaceCount = 0;
	std::size_t hllcFallbackCount = 0;
	double initialDivergenceL2 = 0.0;
	double finalDivergenceL2 = 0.0;
	double divergenceReductionRatio = 0.0;
	double minimumDensity = 0.0;
	double maximumDensity = 0.0;
	double minimumPressure = 0.0;
	double eventStateChangeL1 = 0.0;
	double bulkStateChangeL1 = 0.0;
	double wallInitialDivergenceL2 = 0.0;
	double wallFinalDivergenceL2 = 0.0;
	double wallDivergenceReductionRatio = 0.0;
	double wallNormalVelocityMaximum = 0.0;
	bool variableDensityPassed = false;
	bool hllcProjectionCouplingPassed = false;
	bool crossRouteFluxPassed = false;
	bool conservationPassed = false;
	bool positivityPassed = false;
	bool wallProjectionPassed = false;
	bool productionSolverImplemented = false;
	bool passed = false;
};

HybridProjectionCoupling2DSummary RunHybridProjectionCoupling2D();
bool WriteHybridProjectionCoupling2D(std::ostream &output);

} // namespace omni::atmospherebench
