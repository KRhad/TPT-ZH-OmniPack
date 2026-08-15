#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "OmniPhysicalScale.h"

// OmniAtmosphere is deliberately independent from Legacy Air. It owns an
// authoritative conservative state and exposes derived primitive quantities.
// The 1.0.7 runtime generation uses common multi-species ideal-gas channels;
// Legacy Air remains the Classic-mode authority.

enum class OmniAtmosphereBoundary : uint8_t
{
	Sealed,
	Open,
	Periodic,
};

enum class OmniAtmosphereExecution : uint8_t
{
	RuntimeLowMach,
	ReferenceCompressible,
};

enum class OmniAtmosphereSpeciesPhase : uint8_t
{
	Gas,
	LiquidAerosol,
};

struct OmniAtmosphereSpeciesDefinition
{
	const char *id = "";
	double molarMassKgPerMol = 0.0;
	double specificHeatCpJKgK = 0.0;
	double thermalConductivityWMK = 0.0;
	double diffusionCoefficientM2S = 0.0;
	OmniAtmosphereSpeciesPhase phase = OmniAtmosphereSpeciesPhase::Gas;
};

enum OmniCommonAtmosphereSpecies : std::size_t
{
	OMNI_SPECIES_N2,
	OMNI_SPECIES_O2,
	OMNI_SPECIES_AR,
	OMNI_SPECIES_CO2,
	OMNI_SPECIES_H2O,
	OMNI_COMMON_SPECIES_COUNT,
};

std::vector<OmniAtmosphereSpeciesDefinition> OmniDefaultAtmosphereSpecies();
std::vector<double> OmniEarthLikeAtmosphereMassFractions();

struct OmniAtmosphereScale
{
	double pixelLengthM = OmniPhysicalScale::PixelLengthM;
	double cellLengthM = OmniPhysicalScale::CellLengthM;
	double effectiveDepthM = OmniPhysicalScale::EffectiveDepthM;
	double timestepS = OmniPhysicalScale::TimestepS;

	double cellVolumeM3() const
	{
		return cellLengthM * cellLengthM * effectiveDepthM;
	}

	double faceAreaM2() const
	{
		return cellLengthM * effectiveDepthM;
	}
};

struct OmniAtmosphereConfig
{
	std::size_t width = 1;
	std::size_t height = 1;
	OmniAtmosphereScale scale{};
	double gamma = OmniPhysicalScale::Gamma;
	double gasConstant = OmniPhysicalScale::GasConstantJKgK;
	double referenceDensity = OmniPhysicalScale::ReferenceDensityKgM3;
	double referenceTemperature = OmniPhysicalScale::ReferenceTemperatureK;
	double referencePressure = OmniPhysicalScale::ReferencePressurePa;
	double legacyPressureScalePa = OmniPhysicalScale::LegacyPressureScalePa;
	double cfl = 0.45;
	double densityFloor = 1.0e-9;
	double pressureFloor = 1.0e-3;
	double internalEnergyFloor = 1.0e-3;
	std::vector<OmniAtmosphereSpeciesDefinition> species = OmniDefaultAtmosphereSpecies();
	std::vector<double> referenceMassFractions = OmniEarthLikeAtmosphereMassFractions();
	bool speciesDiffusion = true;
	bool thermalConduction = true;
	bool waterPhaseEquilibrium = true;
	double gravityY = 0.0;
	std::size_t maximumRuntimeSubsteps = 4;
	std::size_t maximumReferenceSubsteps = 100000;
	OmniAtmosphereBoundary boundary = OmniAtmosphereBoundary::Sealed;
	OmniAtmosphereExecution execution = OmniAtmosphereExecution::RuntimeLowMach;
};

struct OmniAtmosphereConservative
{
	double density = 0.0;
	double momentumX = 0.0;
	double momentumY = 0.0;
	double totalEnergy = 0.0;
};

struct OmniAtmospherePrimitive
{
	double density = 0.0;
	double velocityX = 0.0;
	double velocityY = 0.0;
	double pressure = 0.0;
	double temperature = 0.0;
	double soundSpeed = 0.0;
	double mixtureGasConstant = 0.0;
	double mixtureGamma = 0.0;
	double saturationPressurePa = 0.0;
	double waterPartialPressurePa = 0.0;
	double relativeHumidity = 0.0;
	double condensedWaterDensity = 0.0;
	bool finite = false;
};

// Validate one serialized conservative cell before it can enter or leave an
// OPS payload.  Numeric validity is always required; when cellMarkedValid is
// true the complete mixture EOS/latent-energy/pressure-floor feasibility is
// required as well.  Keeping this contract here makes GameSave and runtime
// restore use exactly the same physical acceptance rule.
bool OmniValidateSerializedAtmosphereCell(
	const OmniAtmosphereConfig &config,
	std::span<const double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double totalEnergy,
	double condensedWaterMassDensity,
	bool cellMarkedValid = true);

struct OmniSerializedAtmosphereCellMigration
{
	bool densityAdjusted = false;
	bool energyAdjusted = false;
};

// State version 2 used a weaker physical contract. Parse it using that exact
// legacy contract, then atomically canonicalize the cell to the strict current
// contract before it can enter GameSave or Simulation state. On failure neither
// speciesMassDensity nor totalEnergy is modified.
bool OmniMigrateLegacySerializedAtmosphereCellV2(
	const OmniAtmosphereConfig &config,
	std::span<double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double &totalEnergy,
	double condensedWaterMassDensity,
	bool cellMarkedValid = true,
	OmniSerializedAtmosphereCellMigration *migration = nullptr);

// A caller-owned condensed parcel may react with atmosphere species. This
// explicit transaction is the only 1.0.8 path that may change multiple gas
// channels and chemical energy in one commit. Species deltas are kilograms,
// not densities; positive values enter the atmosphere and negative values are
// consumed from it. The parcel's physical velocity carries its mass, momentum
// and kinetic energy into the gas.
struct OmniAtmosphereReactionTransfer
{
	bool committed = false;
	double gasMassDeltaKg = 0.0;
	double sourceMomentumX = 0.0;
	double sourceMomentumY = 0.0;
	double sensibleEnergyDeltaJ = 0.0;
	double sourceKineticEnergyJ = 0.0;
	double chemicalEnergyJ = 0.0;
	double totalEnergyDeltaJ = 0.0;
};

struct OmniAtmosphereLedger
{
	double initialMassKg = 0.0;
	double finalMassKg = 0.0;
	double initialMomentumX = 0.0;
	double finalMomentumX = 0.0;
	double initialMomentumY = 0.0;
	double finalMomentumY = 0.0;
	double initialEnergyJ = 0.0;
	double finalEnergyJ = 0.0;
	double boundaryMassInKg = 0.0;
	double boundaryMassOutKg = 0.0;
	double boundaryEnergyInJ = 0.0;
	double boundaryEnergyOutJ = 0.0;
	double boundaryMomentumXOut = 0.0;
	double boundaryMomentumYOut = 0.0;
	double sourceMassKg = 0.0;
	double sourceMomentumX = 0.0;
	double sourceMomentumY = 0.0;
	double sourceEnergyJ = 0.0;
	double numericalMassCorrectionKg = 0.0;
	double numericalMomentumXCorrection = 0.0;
	double numericalMomentumYCorrection = 0.0;
	double numericalEnergyCorrectionJ = 0.0;
	std::vector<double> initialSpeciesMassKg;
	std::vector<double> finalSpeciesMassKg;
	std::vector<double> sourceSpeciesMassKg;
	std::vector<double> boundarySpeciesInKg;
	std::vector<double> boundarySpeciesOutKg;
	std::vector<double> numericalSpeciesCorrectionKg;
	double initialCondensedWaterMassKg = 0.0;
	double finalCondensedWaterMassKg = 0.0;
	double phaseTransferWaterMassKg = 0.0;
	double phaseTransferLatentEnergyJ = 0.0;
	double thermalConductionEnergyResidualJ = 0.0;
	uint64_t densityFloorHits = 0;
	uint64_t pressureFloorHits = 0;
	uint64_t energyFloorHits = 0;
	uint64_t nonFiniteCells = 0;
	uint64_t substeps = 0;
	double requestedTimestepS = 0.0;
	double advancedTimestepS = 0.0;
	bool timestepLimited = false;
	bool acousticRoute = false;

	double massResidualKg() const
	{
		return finalMassKg - initialMassKg - sourceMassKg - boundaryMassInKg + boundaryMassOutKg - numericalMassCorrectionKg;
	}

	double energyResidualJ() const
	{
		return finalEnergyJ - initialEnergyJ - sourceEnergyJ - boundaryEnergyInJ + boundaryEnergyOutJ - numericalEnergyCorrectionJ;
	}

	double momentumXResidual() const
	{
		return finalMomentumX - initialMomentumX - sourceMomentumX + boundaryMomentumXOut - numericalMomentumXCorrection;
	}

	double momentumYResidual() const
	{
		return finalMomentumY - initialMomentumY - sourceMomentumY + boundaryMomentumYOut - numericalMomentumYCorrection;
	}

	double speciesMassResidualKg(std::size_t index) const
	{
		if (index >= initialSpeciesMassKg.size() || index >= finalSpeciesMassKg.size())
			return 0.0;
		return finalSpeciesMassKg[index] - initialSpeciesMassKg[index] - sourceSpeciesMassKg[index] -
			boundarySpeciesInKg[index] + boundarySpeciesOutKg[index] - numericalSpeciesCorrectionKg[index];
	}
};

// Region save/stamp restores replace authoritative cells, but must not erase
// unrelated sources that were queued earlier in the same simulation tick.
struct OmniAtmosphereRegionRestoreToken
{
	double pendingSourceMassKg = 0.0;
	double pendingSourceMomentumX = 0.0;
	double pendingSourceMomentumY = 0.0;
	double pendingSourceEnergyJ = 0.0;
	std::vector<double> pendingSourceSpeciesMassKg;
};

class OmniAtmosphere
{
public:
	explicit OmniAtmosphere(OmniAtmosphereConfig config);

	const OmniAtmosphereConfig &Config() const { return config; }
	std::size_t Width() const { return config.width; }
	std::size_t Height() const { return config.height; }
	std::size_t CellCount() const { return state.size(); }
	std::size_t SpeciesCount() const { return config.species.size(); }
	const OmniAtmosphereSpeciesDefinition &SpeciesDefinition(std::size_t index) const;

	void ResetUniform(double density, double temperature, double velocityX = 0.0, double velocityY = 0.0);
	void ResetVacuum(double density = 1.0e-8, double temperature = 293.15);
	void SetReferenceState(double density, double temperature);
	void SetBoundaryMode(OmniAtmosphereBoundary boundary);
	void SetExecutionMode(OmniAtmosphereExecution execution) { config.execution = execution; }
	void SetBlocked(std::size_t x, std::size_t y, bool blocked);
	bool IsBlocked(std::size_t x, std::size_t y) const;

	double AvailableThermalEnergyJ(std::size_t x, std::size_t y,
		double minimumTemperatureK = 1.0) const;
	void AddEnergyDensity(std::size_t x, std::size_t y, double joulesPerM3);
	void AddMassDensity(std::size_t x, std::size_t y, double kilogramsPerM3);
	void AddSpeciesMassDensity(std::size_t x, std::size_t y, std::size_t species, double kilogramsPerM3);
	bool ApplyReactionSpeciesTransfer(std::size_t x, std::size_t y,
		const std::vector<double> &speciesMassDeltaKg, double chemicalEnergyJ,
		double parcelVelocityX, double parcelVelocityY,
		OmniAtmosphereReactionTransfer *result = nullptr);
	void SetSpeciesMassFractions(std::size_t x, std::size_t y, const std::vector<double> &massFractions);
	void SetCondensedWaterDensity(std::size_t x, std::size_t y, double kilogramsPerM3);
	bool RestoreSerializedCell(std::size_t x, std::size_t y, const std::vector<double> &speciesMassDensity,
		double momentumX, double momentumY, double totalEnergy, double condensedWaterMassDensity);
	OmniAtmosphereRegionRestoreToken BeginRegionStateRestore() const;
	// Completes a save/stamp region restore without turning the replacement into
	// an external source and without discarding sources queued outside it.
	void FinalizeRegionStateRestore(
		const OmniAtmosphereRegionRestoreToken &token,
		std::span<const std::size_t> restoredCells);
	// Completes a true whole-grid undo restore after RestoreSerializedCell calls.
	// The restored values become the new authoritative baseline rather than an
	// unexplained numerical/source correction on the next ledgered step.
	void FinalizeStateRestore();
	void SetGravityY(double metresPerSecondSquared);
	void SetCell(std::size_t x, std::size_t y, OmniAtmosphereConservative value);
	void ImportLegacyProjection(std::size_t x, std::size_t y, OmniAtmosphereConservative value);

	// Requests one fixed game tick. RuntimeLowMach avoids artificial acoustic CFL
	// work for a uniform quiet bulk; unresolved pressure/source events remain on
	// the compressible route. If the runtime budget is insufficient, only the
	// stable physical interval advances and the explicit slowdown is reported.
	void Step();
	// Deterministic reference advance for contract tests and offline validation.
	void StepReference(double timestepS);

	const OmniAtmosphereConservative &State(std::size_t x, std::size_t y) const;
	OmniAtmospherePrimitive Primitive(std::size_t x, std::size_t y) const;
	double SpeciesMassDensity(std::size_t x, std::size_t y, std::size_t species) const;
	double SpeciesMassFraction(std::size_t x, std::size_t y, std::size_t species) const;
	double SpeciesPartialPressurePa(std::size_t x, std::size_t y, std::size_t species) const;
	double TotalSpeciesMassKg(std::size_t species) const;
	double TotalCondensedWaterMassKg() const;
	const OmniAtmosphereLedger &Ledger() const { return ledger; }

	double TotalMassKg() const;
	double TotalMomentumX() const;
	double TotalMomentumY() const;
	double TotalEnergyJ() const;
	double MinimumDensity() const;
	double MaximumDensity() const;
	double MinimumPressure() const;
	double MaximumPressure() const;
	double MinimumTemperature() const;
	double MaximumTemperature() const;
	uint64_t NonFiniteStateCells() const;

	// Export is a deliberately explicit projection for Legacy renderer/script
	// consumers. Legacy pv/vx/vy are not SI-authoritative fields.
	float LegacyPressure(std::size_t x, std::size_t y) const;
	float LegacyVelocityX(std::size_t x, std::size_t y) const;
	float LegacyVelocityY(std::size_t x, std::size_t y) const;
	float LegacyTemperature(std::size_t x, std::size_t y) const;

private:
	struct Flux
	{
		double density = 0.0;
		double momentumX = 0.0;
		double momentumY = 0.0;
		double totalEnergy = 0.0;
	};

	OmniAtmosphereConfig config;
	std::vector<OmniAtmosphereConservative> state;
	std::vector<OmniAtmosphereConservative> next;
	std::vector<double> speciesState;
	std::vector<double> speciesNext;
	std::vector<double> condensedWaterDensity;
	std::vector<double> condensedWaterNext;
	std::vector<uint8_t> blocked;
	OmniAtmosphereLedger ledger{};
	bool pendingEvent = false;
	bool compressibleActive = false;
	bool transportActive = false;
	bool phaseActive = false;
	double pendingSourceMassKg = 0.0;
	double pendingSourceMomentumX = 0.0;
	double pendingSourceMomentumY = 0.0;
	double pendingSourceEnergyJ = 0.0;
	std::vector<double> pendingSourceSpeciesMassKg;

	std::size_t Index(std::size_t x, std::size_t y) const { return y * config.width + x; }
	std::size_t SpeciesIndex(std::size_t cell, std::size_t species) const { return cell * config.species.size() + species; }
	OmniAtmosphereConservative AmbientState() const;
	std::vector<double> AmbientSpeciesState() const;
	OmniAtmospherePrimitive Derive(const OmniAtmosphereConservative &value) const;
	OmniAtmospherePrimitive Derive(std::size_t cell, const OmniAtmosphereConservative &value) const;
	Flux PhysicalFlux(const OmniAtmosphereConservative &value, bool xDirection) const;
	Flux PhysicalFlux(std::size_t cell, const OmniAtmosphereConservative &value, bool xDirection) const;
	Flux RusanovFlux(const OmniAtmosphereConservative &left, const OmniAtmosphereConservative &right, bool xDirection, bool acoustic) const;
	Flux RusanovFlux(std::size_t leftCell, std::size_t rightCell, const OmniAtmosphereConservative &left, const OmniAtmosphereConservative &right, bool xDirection, bool acoustic) const;
	void AdvanceOnce(double dt, bool acoustic);
	void DiffuseSpeciesAndHeat(double dt);
	void ApplyGravity(double dt);
	void EquilibrateWaterPhase();
	void ApplyFloors(OmniAtmosphereConservative &value, bool recordCorrection = true);
	void NormalizeSpecies(std::size_t cell, double targetDensity, bool recordCorrection = true);
	double SpeciesFlux(std::size_t species, std::size_t leftCell, std::size_t rightCell,
		const OmniAtmosphereConservative &left, const OmniAtmosphereConservative &right,
		bool xDirection, bool acoustic) const;
	void RecordBoundarySpeciesFlux(const std::vector<double> &flux, bool positiveOutward, double dt);
	void BeginLedger();
	void FinishLedger();
	void RecordBoundaryFlux(const Flux &flux, bool xDirection, bool positiveOutward, double dt);
	void ResetState(std::vector<OmniAtmosphereConservative> &target, double density, double temperature, double velocityX, double velocityY);
	bool CompressibleFeaturesPresent() const;
};
