#pragma once

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct Phase5SelectionSummary
{
	double physicalSecondsPerTick = 0.0;
	double referenceAtmosphereBudgetMilliseconds = 0.0;
	double referenceSoundSpeedMPerS = 0.0;
	double multigridReferenceGridElapsedMilliseconds = 0.0;
	double multigridReferenceGridDivergenceRatio = 0.0;
	double hybridMaximumEventFraction = 0.0;
	double coupledProjectionDivergenceRatio = 0.0;
	std::size_t physicalAcousticDomainCells = 0;
	std::size_t coupledProjectionCrossRouteFaces = 0;
	bool physicalScaleValid = false;
	bool presentationIndependent = false;
	bool uniformAcousticScalingUsed = false;
	bool strictDoubleReferenceSelected = false;
	bool lowMachComponentSelected = false;
	bool compressibleComponentSelected = false;
	bool lowMachTargetMatrixPassed = false;
	bool physicalAcousticDomainExceedsBenchmark = false;
	bool coupledProjectionWallPassed = false;
	bool hybridRoutingEvidencePassed = false;
	bool coupledProjectionEvidencePassed = false;
	bool hllcPhysicsMatrixPassed = false;
	bool speciesTransportEvidencePassed = false;
	bool legacyControlEvidencePassed = false;
	bool lbmComparisonEvidencePassed = false;
	bool productionSolverImplemented = false;
	bool selectionValidated = false;
};

Phase5SelectionSummary RunPhase5Selection();
bool WritePhase5Selection(std::ostream &output);

} // namespace omni::atmospherebench
