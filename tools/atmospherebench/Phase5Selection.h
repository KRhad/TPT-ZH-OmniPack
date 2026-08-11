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
	std::size_t physicalAcousticDomainCells = 0;
	bool physicalScaleValid = false;
	bool presentationIndependent = false;
	bool uniformAcousticScalingUsed = false;
	bool strictDoubleReferenceSelected = false;
	bool lowMachComponentSelected = false;
	bool compressibleComponentSelected = false;
	bool productionSolverImplemented = false;
	bool selectionValidated = false;
};

Phase5SelectionSummary RunPhase5Selection();
bool WritePhase5Selection(std::ostream &output);

} // namespace omni::atmospherebench
