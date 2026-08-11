#pragma once

#include "AtmosphereBench.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct LbmD2Q9ProbeSummary
{
	BenchmarkCase benchmarkCase;
	NumericalCorrectionLedger corrections{};
	double relaxationTime = 0.0;
	double kinematicViscosity = 0.0;
	double initialMass = 0.0;
	double finalMass = 0.0;
	double massDrift = 0.0;
	double initialMomentumX = 0.0;
	double finalMomentumX = 0.0;
	double initialMomentumY = 0.0;
	double finalMomentumY = 0.0;
	double momentumXDrift = 0.0;
	double momentumYDrift = 0.0;
	double minimumDensity = 0.0;
	double maximumDensity = 0.0;
	double minimumPopulation = 0.0;
	double maximumMach = 0.0;
	double initialShearAmplitude = 0.0;
	double finalShearAmplitude = 0.0;
	double expectedShearAmplitude = 0.0;
	double shearAmplitudeRelativeError = 0.0;
	double stateChangeL1 = 0.0;
	double stateAndScratchBytesPerCell = 0.0;
	std::size_t stateAndScratchBytesTotal = 0;
	bool massConserved = false;
	bool momentumConserved = false;
	bool positivityPreserved = false;
	bool uniformPreserved = false;
	bool shearReferencePassed = false;
	bool passed = false;
};

LbmD2Q9ProbeSummary RunLbmD2Q9Uniform();
LbmD2Q9ProbeSummary RunLbmD2Q9ShearWave();
bool WriteLbmD2Q9UniformProbe(std::ostream &output);
bool WriteLbmD2Q9ShearWaveProbe(std::ostream &output);

} // namespace omni::atmospherebench
