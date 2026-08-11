#pragma once

#include "AtmosphereBench.h"

#include <cstddef>
#include <iosfwd>

namespace omni::atmospherebench
{

struct Hllc2DSpeciesMixingSummary
{
	BenchmarkCase benchmarkCase;
	ConservationLedger gasLedger;
	NumericalCorrectionLedger corrections{};
	double maximumCfl = 0.0;
	double minimumDensity = 0.0;
	double minimumPressure = 0.0;
	double initialSpeciesAMass = 0.0;
	double finalSpeciesAMass = 0.0;
	double initialSpeciesBMass = 0.0;
	double finalSpeciesBMass = 0.0;
	double speciesAMassDrift = 0.0;
	double speciesBMassDrift = 0.0;
	double minimumSpeciesAFraction = 0.0;
	double maximumSpeciesAFraction = 0.0;
	double initialCompositionTotalVariation = 0.0;
	double finalCompositionTotalVariation = 0.0;
	double compositionStateChangeL1 = 0.0;
	double stateAndFluxScratchBytesPerCell = 0.0;
	std::size_t stateAndFluxScratchBytesTotal = 0;
	std::size_t initialMixedCellCount = 0;
	std::size_t finalMixedCellCount = 0;
	std::size_t fluxFallbackCount = 0;
	bool gasLedgerCloses = false;
	bool speciesLedgerCloses = false;
	bool speciesBoundsPreserved = false;
	bool compositionEvolved = false;
	bool mixedRegionFormed = false;
	bool passed = false;
};

Hllc2DSpeciesMixingSummary RunHllc2DSpeciesMixing();
bool WriteHllc2DSpeciesMixingProbe(std::ostream &output);

} // namespace omni::atmospherebench
