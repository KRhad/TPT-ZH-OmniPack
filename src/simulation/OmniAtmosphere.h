#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "OmniPhysicalScale.h"

// OmniAtmosphere is deliberately independent from Legacy Air. It owns an
// authoritative conservative state and exposes derived primitive quantities.
// The first runtime generation is single-species ideal-gas Euler physics; the
// species channels and particle coupling are intentionally reserved for 1.0.7+.

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
	bool finite = false;
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
};

class OmniAtmosphere
{
public:
	explicit OmniAtmosphere(OmniAtmosphereConfig config);

	const OmniAtmosphereConfig &Config() const { return config; }
	std::size_t Width() const { return config.width; }
	std::size_t Height() const { return config.height; }
	std::size_t CellCount() const { return state.size(); }

	void ResetUniform(double density, double temperature, double velocityX = 0.0, double velocityY = 0.0);
	void ResetVacuum(double density = 1.0e-8, double temperature = 293.15);
	void SetReferenceState(double density, double temperature);
	void SetBoundaryMode(OmniAtmosphereBoundary boundary);
	void SetExecutionMode(OmniAtmosphereExecution execution) { config.execution = execution; }
	void SetBlocked(std::size_t x, std::size_t y, bool blocked);
	bool IsBlocked(std::size_t x, std::size_t y) const;

	void AddEnergyDensity(std::size_t x, std::size_t y, double joulesPerM3);
	void AddMassDensity(std::size_t x, std::size_t y, double kilogramsPerM3);
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
	const OmniAtmosphereLedger &Ledger() const { return ledger; }

	double TotalMassKg() const;
	double TotalMomentumX() const;
	double TotalMomentumY() const;
	double TotalEnergyJ() const;
	double MinimumDensity() const;
	double MinimumPressure() const;

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
	std::vector<uint8_t> blocked;
	OmniAtmosphereLedger ledger{};
	bool pendingEvent = false;
	bool compressibleActive = false;
	double pendingSourceMassKg = 0.0;
	double pendingSourceMomentumX = 0.0;
	double pendingSourceMomentumY = 0.0;
	double pendingSourceEnergyJ = 0.0;

	std::size_t Index(std::size_t x, std::size_t y) const { return y * config.width + x; }
	OmniAtmosphereConservative AmbientState() const;
	OmniAtmospherePrimitive Derive(const OmniAtmosphereConservative &value) const;
	Flux PhysicalFlux(const OmniAtmosphereConservative &value, bool xDirection) const;
	Flux RusanovFlux(const OmniAtmosphereConservative &left, const OmniAtmosphereConservative &right, bool xDirection, bool acoustic) const;
	void AdvanceOnce(double dt, bool acoustic);
	void ApplyFloors(OmniAtmosphereConservative &value, bool recordCorrection = true);
	void BeginLedger();
	void FinishLedger();
	void RecordBoundaryFlux(const Flux &flux, bool xDirection, bool positiveOutward, double dt);
	void ResetState(std::vector<OmniAtmosphereConservative> &target, double density, double temperature, double velocityX, double velocityY);
	bool CompressibleFeaturesPresent() const;
};
