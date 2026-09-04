#include "OmniAtmosphere.h"
#include "OmniCompute.h"
#include "OmniThermal.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace
{
constexpr double UniversalGasConstant = 8.31446261815324;

bool Finite(double value)
{
	// Keep serialized-state validation sound under fast-math callers too.  A
	// floating-point comparison or std::isfinite can be optimized away when a
	// translation unit is built with finite-only assumptions.
	return (std::bit_cast<uint64_t>(value) & UINT64_C(0x7FF0000000000000)) !=
		UINT64_C(0x7FF0000000000000);
}

double Square(double value)
{
	return value * value;
}

double SpeciesGasConstant(const OmniAtmosphereSpeciesDefinition &species)
{
	return species.phase == OmniAtmosphereSpeciesPhase::Gas
		? UniversalGasConstant / species.molarMassKgPerMol
		: 0.0;
}

double SpeciesCv(const OmniAtmosphereSpeciesDefinition &species)
{
	return species.phase == OmniAtmosphereSpeciesPhase::Gas
		? species.specificHeatCpJKgK - SpeciesGasConstant(species)
		: species.specificHeatCpJKgK;
}

std::vector<double> NormalizeFractions(std::vector<double> fractions, std::size_t count)
{
	if (fractions.size() != count)
		fractions.assign(count, 0.0);
	double total = 0.0;
	for (double &fraction : fractions)
	{
		if (!Finite(fraction) || fraction < 0.0)
			fraction = 0.0;
		total += fraction;
	}
	if (!(total > 0.0))
	{
		if (count)
			fractions[0] = 1.0;
		return fractions;
	}
	for (double &fraction : fractions)
		fraction /= total;
	return fractions;
}

double MixtureGasConstant(
	const std::vector<OmniAtmosphereSpeciesDefinition> &species,
	const std::vector<double> &fractions)
{
	double result = 0.0;
	for (std::size_t i = 0; i < species.size() && i < fractions.size(); ++i)
		result += fractions[i] * SpeciesGasConstant(species[i]);
	return result;
}

double MixtureCv(
	const std::vector<OmniAtmosphereSpeciesDefinition> &species,
	const std::vector<double> &fractions)
{
	double result = 0.0;
	for (std::size_t i = 0; i < species.size() && i < fractions.size(); ++i)
		result += fractions[i] * SpeciesCv(species[i]);
	return result;
}

double MixtureCp(
	const std::vector<OmniAtmosphereSpeciesDefinition> &species,
	const std::vector<double> &fractions)
{
	double result = 0.0;
	for (std::size_t i = 0; i < species.size() && i < fractions.size(); ++i)
		result += fractions[i] * species[i].specificHeatCpJKgK;
	return result;
}

bool ValidateSerializedAtmosphereCell(
	const OmniAtmosphereConfig &config,
	std::span<const double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double totalEnergy,
	double condensedWaterMassDensity,
	bool cellMarkedValid)
{
	if (speciesMassDensity.size() != config.species.size() ||
		!Finite(momentumX) || !Finite(momentumY) || !Finite(totalEnergy) ||
		totalEnergy < 0.0 || !Finite(condensedWaterMassDensity) ||
		condensedWaterMassDensity < 0.0 || config.species.size() <= OMNI_SPECIES_H2O ||
		!Finite(config.pressureFloor) || config.pressureFloor <= 0.0 ||
		!Finite(config.internalEnergyFloor) || config.internalEnergyFloor <= 0.0 ||
		!Finite(config.densityFloor) || config.densityFloor <= 0.0)
	{
		return false;
	}

	double density = 0.0;
	double heatCapacityDensity = condensedWaterMassDensity * OmniThermal::LiquidSpecificHeatJKgK;
	double gasConstantDensity = 0.0;
	double waterDensity = 0.0;
	for (std::size_t species = 0; species < speciesMassDensity.size(); ++species)
	{
		const double value = speciesMassDensity[species];
		if (!Finite(value) || value < 0.0)
			return false;
		density += value;
		heatCapacityDensity += value * SpeciesCv(config.species[species]);
		gasConstantDensity += value * SpeciesGasConstant(config.species[species]);
		if (species == OMNI_SPECIES_H2O)
			waterDensity = value;
	}
	if (!Finite(density) || !Finite(heatCapacityDensity) || !Finite(gasConstantDensity) ||
		!Finite(waterDensity))
	{
		return false;
	}
	// Invalid-mask cells are allowed to carry a finite, non-negative placeholder
	// payload, but no physical state is accepted until the mask marks it valid.
	if (!cellMarkedValid)
		return true;
	if (!(density >= config.densityFloor) || !(heatCapacityDensity > 0.0) ||
		!(gasConstantDensity > 0.0))
	{
		return false;
	}
	const double kinetic = 0.5 * (Square(momentumX) + Square(momentumY)) / density;
	const double latentEnergyDensity = waterDensity * OmniThermal::LatentHeatVaporizationJPerKg;
	if (!Finite(kinetic) || !Finite(latentEnergyDensity))
		return false;
	const double minimumTemperatureK = std::max(
		1.0, config.pressureFloor / gasConstantDensity);
	const double requiredSensibleEnergyDensity = std::max(
		config.internalEnergyFloor, heatCapacityDensity * minimumTemperatureK);
	const double requiredTotalEnergy = kinetic + latentEnergyDensity + requiredSensibleEnergyDensity;
	if (!Finite(minimumTemperatureK) || !Finite(requiredSensibleEnergyDensity) ||
		!Finite(requiredTotalEnergy) || totalEnergy < requiredTotalEnergy)
	{
		return false;
	}
	const double temperature =
		(totalEnergy - kinetic - latentEnergyDensity) / heatCapacityDensity;
	const double pressure = temperature * gasConstantDensity;
	return Finite(temperature) && temperature >= minimumTemperatureK &&
		Finite(pressure) && pressure >= config.pressureFloor;
}

bool ValidateLegacySerializedAtmosphereCellV2(
	const OmniAtmosphereConfig &config,
	std::span<const double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double totalEnergy,
	double condensedWaterMassDensity,
	bool cellMarkedValid)
{
	if (speciesMassDensity.size() != config.species.size() ||
		config.species.size() <= OMNI_SPECIES_H2O ||
		!Finite(momentumX) || !Finite(momentumY) || !Finite(totalEnergy) ||
		!Finite(condensedWaterMassDensity) || condensedWaterMassDensity < 0.0)
	{
		return false;
	}
	double density = 0.0;
	for (double value : speciesMassDensity)
	{
		if (!Finite(value) || value < 0.0)
			return false;
		density += value;
	}
	if (!Finite(density))
		return false;
	// v2 only required positive gas density and positive total energy for a
	// cell marked valid. Invalid-mask cells intentionally carried placeholders.
	return !cellMarkedValid || (density > 0.0 && totalEnergy > 0.0);
}

bool ComputeSerializedAtmosphereRequiredEnergy(
	const OmniAtmosphereConfig &config,
	std::span<const double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double condensedWaterMassDensity,
	double &requiredTotalEnergy)
{
	if (speciesMassDensity.size() != config.species.size() ||
		config.species.size() <= OMNI_SPECIES_H2O ||
		!Finite(momentumX) || !Finite(momentumY) ||
		!Finite(condensedWaterMassDensity) || condensedWaterMassDensity < 0.0 ||
		!Finite(config.pressureFloor) || config.pressureFloor <= 0.0 ||
		!Finite(config.internalEnergyFloor) || config.internalEnergyFloor <= 0.0 ||
		!Finite(config.densityFloor) || config.densityFloor <= 0.0)
	{
		return false;
	}
	double density = 0.0;
	double heatCapacityDensity = condensedWaterMassDensity * OmniThermal::LiquidSpecificHeatJKgK;
	double gasConstantDensity = 0.0;
	double waterDensity = 0.0;
	for (std::size_t species = 0; species < speciesMassDensity.size(); ++species)
	{
		const double value = speciesMassDensity[species];
		if (!Finite(value) || value < 0.0)
			return false;
		density += value;
		heatCapacityDensity += value * SpeciesCv(config.species[species]);
		gasConstantDensity += value * SpeciesGasConstant(config.species[species]);
		if (species == OMNI_SPECIES_H2O)
			waterDensity = value;
	}
	if (!Finite(density) || density < config.densityFloor ||
		!Finite(heatCapacityDensity) || !(heatCapacityDensity > 0.0) ||
		!Finite(gasConstantDensity) || !(gasConstantDensity > 0.0) ||
		!Finite(waterDensity))
	{
		return false;
	}
	const double kinetic = 0.5 * (Square(momentumX) + Square(momentumY)) / density;
	const double latentEnergyDensity =
		waterDensity * OmniThermal::LatentHeatVaporizationJPerKg;
	const double minimumTemperatureK = std::max(
		1.0, config.pressureFloor / gasConstantDensity);
	const double requiredSensibleEnergyDensity = std::max(
		config.internalEnergyFloor, heatCapacityDensity * minimumTemperatureK);
	requiredTotalEnergy = kinetic + latentEnergyDensity + requiredSensibleEnergyDensity;
	return Finite(kinetic) && Finite(latentEnergyDensity) &&
		Finite(minimumTemperatureK) && Finite(requiredSensibleEnergyDensity) &&
		Finite(requiredTotalEnergy);
}

double SumSpeciesMassDensity(std::span<const double> speciesMassDensity)
{
	double result = 0.0;
	for (double value : speciesMassDensity)
		result += value;
	return result;
}
}

bool OmniValidateSerializedAtmosphereCell(
	const OmniAtmosphereConfig &config,
	std::span<const double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double totalEnergy,
	double condensedWaterMassDensity,
	bool cellMarkedValid)
{
	return ValidateSerializedAtmosphereCell(
		config, speciesMassDensity, momentumX, momentumY, totalEnergy,
		condensedWaterMassDensity, cellMarkedValid);
}

bool OmniMigrateLegacySerializedAtmosphereCellV2(
	const OmniAtmosphereConfig &config,
	std::span<double> speciesMassDensity,
	double momentumX,
	double momentumY,
	double &totalEnergy,
	double condensedWaterMassDensity,
	bool cellMarkedValid,
	OmniSerializedAtmosphereCellMigration *migration)
{
	if (migration)
		*migration = {};
	if (!ValidateLegacySerializedAtmosphereCellV2(
			config, std::span<const double>(speciesMassDensity.data(), speciesMassDensity.size()),
			momentumX, momentumY, totalEnergy, condensedWaterMassDensity, cellMarkedValid))
	{
		return false;
	}

	// Work in temporaries so a failed migration cannot leave a half-updated cell.
	std::vector<double> canonical(speciesMassDensity.begin(), speciesMassDensity.end());
	double canonicalEnergy = totalEnergy;
	bool densityAdjusted = false;
	bool energyAdjusted = false;
	if (!cellMarkedValid)
	{
		// v2 permitted negative placeholder energy for invalid-mask cells; v3's
		// numeric contract is non-negative even when the cell is not physical.
		if (canonicalEnergy < 0.0)
		{
			canonicalEnergy = 0.0;
			energyAdjusted = true;
		}
	}
	else
	{
		const double density = SumSpeciesMassDensity(
			std::span<const double>(canonical.data(), canonical.size()));
		if (!Finite(density) || !(density > 0.0))
			return false;
		if (density < config.densityFloor)
		{
			for (double &value : canonical)
			{
				// Divide first so even a positive subnormal v2 density can be
				// migrated without overflowing an intermediate scale factor.
				value = (value / density) * config.densityFloor;
				if (!Finite(value) || value < 0.0)
					return false;
			}
			// Correct the deterministic residual on the largest channel. This
			// keeps the composition stable while guaranteeing the density floor is
			// reached despite floating-point summation roundoff.
			std::size_t correctionIndex = 0;
			for (std::size_t index = 1; index < canonical.size(); ++index)
			{
				if (canonical[index] > canonical[correctionIndex])
					correctionIndex = index;
			}
			const double scaledDensity = SumSpeciesMassDensity(
				std::span<const double>(canonical.data(), canonical.size()));
			const double corrected = canonical[correctionIndex] +
				(config.densityFloor - scaledDensity);
			if (!Finite(corrected) || corrected < 0.0)
				return false;
			canonical[correctionIndex] = corrected;
			for (int attempt = 0; attempt < 8 &&
				SumSpeciesMassDensity(std::span<const double>(canonical.data(), canonical.size())) <
					config.densityFloor; ++attempt)
			{
				canonical[correctionIndex] = std::nextafter(
					canonical[correctionIndex], std::numeric_limits<double>::infinity());
			}
			if (SumSpeciesMassDensity(std::span<const double>(canonical.data(), canonical.size())) <
				config.densityFloor)
			{
				return false;
			}
			densityAdjusted = true;
		}
		double requiredTotalEnergy = 0.0;
		if (!ComputeSerializedAtmosphereRequiredEnergy(
			config, std::span<const double>(canonical.data(), canonical.size()),
			momentumX, momentumY, condensedWaterMassDensity, requiredTotalEnergy))
		{
			return false;
		}
		if (canonicalEnergy < requiredTotalEnergy)
		{
			canonicalEnergy = requiredTotalEnergy;
			energyAdjusted = true;
		}
	}
	auto canonicalIsValid = [&] {
		return OmniValidateSerializedAtmosphereCell(
			config, std::span<const double>(canonical.data(), canonical.size()),
			momentumX, momentumY, canonicalEnergy, condensedWaterMassDensity,
			cellMarkedValid);
	};
	// Exact required-energy arithmetic may round down after a large kinetic or
	// latent term is subtracted again by the strict validator. Raise by at most
	// a handful of representable steps; any structural failure still rejects.
	for (int attempt = 0; cellMarkedValid && attempt < 8 && !canonicalIsValid(); ++attempt)
	{
		canonicalEnergy = std::nextafter(
			canonicalEnergy, std::numeric_limits<double>::infinity());
		energyAdjusted = true;
	}
	if (!canonicalIsValid())
	{
		return false;
	}
	std::copy(canonical.begin(), canonical.end(), speciesMassDensity.begin());
	totalEnergy = canonicalEnergy;
	if (migration)
	{
		migration->densityAdjusted = densityAdjusted;
		migration->energyAdjusted = energyAdjusted;
	}
	return true;
}

std::vector<OmniAtmosphereSpeciesDefinition> OmniDefaultAtmosphereSpecies()
{
	return {
		{ "N2", 0.0280134, 1040.0, 0.0258, 2.00e-5, OmniAtmosphereSpeciesPhase::Gas },
		{ "O2", 0.0319988, 918.0, 0.0263, 2.00e-5, OmniAtmosphereSpeciesPhase::Gas },
		{ "Ar", 0.0399480, 520.3, 0.0177, 1.80e-5, OmniAtmosphereSpeciesPhase::Gas },
		{ "CO2", 0.0440095, 844.0, 0.0166, 1.60e-5, OmniAtmosphereSpeciesPhase::Gas },
		{ "H2O", 0.01801528, 1850.0, 0.0190, 2.60e-5, OmniAtmosphereSpeciesPhase::Gas },
	};
}

std::vector<double> OmniEarthLikeAtmosphereMassFractions()
{
	// Dry-air mole fractions from the documented atmosphere preset are converted
	// once to mass fractions; runtime never hardcodes an "oxygen exists" flag.
	return { 0.755107198945, 0.231374517735, 0.012880201368, 0.000638081951, 0.0 };
}

OmniAtmosphere::OmniAtmosphere(OmniAtmosphereConfig newConfig):
	config(newConfig),
	state(config.width * config.height),
	next(config.width * config.height),
	speciesState(config.width * config.height * config.species.size(), 0.0),
	speciesNext(config.width * config.height * config.species.size(), 0.0),
	condensedWaterDensity(config.width * config.height, 0.0),
	condensedWaterNext(config.width * config.height, 0.0),
	blocked(config.width * config.height, 0),
	pendingSourceSpeciesMassKg(config.species.size(), 0.0)
{
	if (!config.width || !config.height ||
		!Finite(config.scale.cellLengthM) || config.scale.cellLengthM <= 0.0 ||
		!Finite(config.scale.effectiveDepthM) || config.scale.effectiveDepthM <= 0.0 ||
		!Finite(config.scale.timestepS) || config.scale.timestepS <= 0.0 ||
		!Finite(config.gamma) || config.gamma <= 1.0 ||
		!Finite(config.gasConstant) || config.gasConstant <= 0.0 ||
		!Finite(config.referenceDensity) || config.referenceDensity <= 0.0 ||
		!Finite(config.referenceTemperature) || config.referenceTemperature <= 0.0 ||
		!Finite(config.referencePressure) || config.referencePressure <= 0.0 ||
		!Finite(config.legacyPressureScalePa) || config.legacyPressureScalePa <= 0.0 ||
		!Finite(config.densityFloor) || config.densityFloor <= 0.0 ||
		!Finite(config.pressureFloor) || config.pressureFloor <= 0.0 ||
		!Finite(config.internalEnergyFloor) || config.internalEnergyFloor <= 0.0 ||
		!Finite(config.cfl) || config.cfl <= 0.0 || config.cfl > 1.0 ||
		!config.maximumRuntimeSubsteps || !config.maximumReferenceSubsteps || config.species.empty() ||
		config.referenceMassFractions.size() != config.species.size())
	{
		throw std::invalid_argument("invalid OmniAtmosphere configuration");
	}
	config.referenceMassFractions = NormalizeFractions(config.referenceMassFractions, config.species.size());
	for (const auto &species : config.species)
	{
		if (!species.id || !Finite(species.molarMassKgPerMol) || species.molarMassKgPerMol <= 0.0 ||
			!Finite(species.specificHeatCpJKgK) || species.specificHeatCpJKgK <= 0.0 ||
			!Finite(species.thermalConductivityWMK) || species.thermalConductivityWMK < 0.0 ||
			!Finite(species.diffusionCoefficientM2S) || species.diffusionCoefficientM2S < 0.0 ||
			(species.phase == OmniAtmosphereSpeciesPhase::Gas &&
				SpeciesCv(species) <= 0.0))
			throw std::invalid_argument("invalid OmniAtmosphere species definition");
	}
	const double mixtureR = MixtureGasConstant(config.species, config.referenceMassFractions);
	if (mixtureR <= 0.0)
		throw std::invalid_argument("species mixture has no pressure-bearing gas");
	config.gasConstant = mixtureR;
	config.gamma = MixtureCp(config.species, config.referenceMassFractions) / MixtureCv(config.species, config.referenceMassFractions);
	config.referencePressure = config.referenceDensity * config.gasConstant * config.referenceTemperature;
	ResetUniform(config.referenceDensity, config.referenceTemperature);
}

void OmniAtmosphere::ResetState(
	std::vector<OmniAtmosphereConservative> &target,
	double density,
	double temperature,
	double velocityX,
	double velocityY)
{
	if (!Finite(density) || !Finite(temperature) || !Finite(velocityX) || !Finite(velocityY) ||
		density <= 0.0 || temperature <= 0.0)
	{
		throw std::invalid_argument("invalid OmniAtmosphere uniform state");
	}
	const double pressure = density * config.gasConstant * temperature;
	const double kinetic = 0.5 * density * (Square(velocityX) + Square(velocityY));
	const OmniAtmosphereConservative value{
		density,
		density * velocityX,
		density * velocityY,
		pressure / (config.gamma - 1.0) + kinetic,
	};
	std::fill(target.begin(), target.end(), value);
}

void OmniAtmosphere::ResetUniform(double density, double temperature, double velocityX, double velocityY)
{
	ResetState(state, density, temperature, velocityX, velocityY);
	const auto fractions = NormalizeFractions(config.referenceMassFractions, config.species.size());
	const double mixtureCv = MixtureCv(config.species, fractions);
	for (std::size_t cell = 0; cell < state.size(); ++cell)
	{
		for (std::size_t species = 0; species < config.species.size(); ++species)
			speciesState[SpeciesIndex(cell, species)] = density * fractions[species];
		const double kinetic = 0.5 * density * (Square(velocityX) + Square(velocityY));
		state[cell].density = density;
		state[cell].momentumX = density * velocityX;
		state[cell].momentumY = density * velocityY;
		state[cell].totalEnergy = density * mixtureCv * temperature +
			speciesState[SpeciesIndex(cell, OMNI_SPECIES_H2O)] * OmniThermal::LatentHeatVaporizationJPerKg + kinetic;
	}
	next = state;
	speciesNext = speciesState;
	std::fill(condensedWaterDensity.begin(), condensedWaterDensity.end(), 0.0);
	condensedWaterNext = condensedWaterDensity;
	std::fill(blocked.begin(), blocked.end(), 0);
	ledger = {};
	pendingEvent = false;
	compressibleActive = false;
	transportActive = false;
	phaseActive = config.species.size() > OMNI_SPECIES_H2O &&
		TotalSpeciesMassKg(OMNI_SPECIES_H2O) > 0.0;
	pendingSourceMassKg = 0.0;
	pendingSourceMomentumX = 0.0;
	pendingSourceMomentumY = 0.0;
	pendingSourceEnergyJ = 0.0;
	std::fill(pendingSourceSpeciesMassKg.begin(), pendingSourceSpeciesMassKg.end(), 0.0);
}

void OmniAtmosphere::ResetVacuum(double density, double temperature)
{
	ResetUniform(std::max(density, config.densityFloor), temperature);
}

void OmniAtmosphere::SetReferenceState(double density, double temperature)
{
	if (!Finite(density) || density <= 0.0 || !Finite(temperature) || temperature <= 0.0)
		throw std::invalid_argument("invalid OmniAtmosphere reference state");
	config.referenceDensity = density;
	config.referenceTemperature = temperature;
	config.referencePressure = density * config.gasConstant * temperature;
}

void OmniAtmosphere::SetBoundaryMode(OmniAtmosphereBoundary boundary)
{
	if (config.boundary == boundary)
		return;
	config.boundary = boundary;
	pendingEvent = true;
}

void OmniAtmosphere::SetBlocked(std::size_t x, std::size_t y, bool value)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere blocked-cell coordinate");
	const auto index = Index(x, y);
	const uint8_t nextValue = value ? 1 : 0;
	if (blocked[index] == nextValue)
		return;
	blocked[index] = nextValue;
	pendingEvent = true;
}

bool OmniAtmosphere::IsBlocked(std::size_t x, std::size_t y) const
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere blocked-cell coordinate");
	return blocked[Index(x, y)] != 0;
}

double OmniAtmosphere::AvailableThermalEnergyJ(
	std::size_t x, std::size_t y, double minimumTemperatureK) const
{
	if (x >= config.width || y >= config.height || !Finite(minimumTemperatureK) ||
		minimumTemperatureK <= 0.0)
	{
		throw std::invalid_argument("invalid OmniAtmosphere thermal-energy query");
	}
	const auto index = Index(x, y);
	const auto &cell = state[index];
	if (!Finite(cell.density) || !Finite(cell.momentumX) || !Finite(cell.momentumY) ||
		!Finite(cell.totalEnergy) || !(cell.density > 0.0))
		return 0.0;
	const double kinetic = 0.5 * (Square(cell.momentumX) + Square(cell.momentumY)) / cell.density;
	double heatCapacityDensity = condensedWaterDensity[index] * OmniThermal::LiquidSpecificHeatJKgK;
	double gasConstantDensity = 0.0;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		heatCapacityDensity += speciesState[SpeciesIndex(index, species)] * SpeciesCv(config.species[species]);
		gasConstantDensity += speciesState[SpeciesIndex(index, species)] * SpeciesGasConstant(config.species[species]);
	}
	const double latentEnergyDensity =
		speciesState[SpeciesIndex(index, OMNI_SPECIES_H2O)] * OmniThermal::LatentHeatVaporizationJPerKg;
	const double physicalMinimumTemperatureK = std::max(
		minimumTemperatureK,
		config.pressureFloor / std::max(gasConstantDensity, 1.0e-300));
	const double requiredSensibleEnergyDensity = std::max(
		config.internalEnergyFloor, heatCapacityDensity * physicalMinimumTemperatureK);
	const double minimumEnergyDensity = kinetic + latentEnergyDensity +
		requiredSensibleEnergyDensity;
	if (!Finite(kinetic) || !Finite(heatCapacityDensity) || !(heatCapacityDensity > 0.0) ||
		!Finite(gasConstantDensity) || !(gasConstantDensity > 0.0) ||
		!Finite(physicalMinimumTemperatureK) || !Finite(latentEnergyDensity) ||
		!Finite(requiredSensibleEnergyDensity) ||
		!Finite(minimumEnergyDensity))
		return 0.0;
	return std::max((cell.totalEnergy - minimumEnergyDensity) * config.scale.cellVolumeM3(), 0.0);
}

void OmniAtmosphere::AddEnergyDensity(std::size_t x, std::size_t y, double joulesPerM3)
{
	if (x >= config.width || y >= config.height || !Finite(joulesPerM3))
		throw std::invalid_argument("invalid OmniAtmosphere energy source");
	const auto index = Index(x, y);
	const double volume = config.scale.cellVolumeM3();
	const double energyJ = joulesPerM3 * volume;
	const double nextTotalEnergy = state[index].totalEnergy + joulesPerM3;
	const double nextPendingSourceEnergyJ = pendingSourceEnergyJ + energyJ;
	if (!Finite(energyJ) || !Finite(nextTotalEnergy) || !Finite(nextPendingSourceEnergyJ))
		throw std::invalid_argument("OmniAtmosphere energy source would create a non-finite state");
	if (joulesPerM3 < 0.0 &&
		-energyJ > AvailableThermalEnergyJ(x, y) + 1.0e-15)
	{
		throw std::invalid_argument("OmniAtmosphere energy source exceeds available thermal energy");
	}
	state[index].totalEnergy = nextTotalEnergy;
	pendingSourceEnergyJ = nextPendingSourceEnergyJ;
	pendingEvent = true;
	transportActive = true;
}

void OmniAtmosphere::AddMassDensity(std::size_t x, std::size_t y, double kilogramsPerM3)
{
	if (x >= config.width || y >= config.height || !Finite(kilogramsPerM3))
		throw std::invalid_argument("invalid OmniAtmosphere mass source");
	const auto index = Index(x, y);
	auto &cell = state[index];
	const auto ambient = AmbientState();
	const double ambientSpecificEnergy = ambient.totalEnergy / ambient.density;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		const double added = kilogramsPerM3 * config.referenceMassFractions[species];
		speciesState[SpeciesIndex(index, species)] += added;
		pendingSourceSpeciesMassKg[species] += added * config.scale.cellVolumeM3();
	}
	cell.density += kilogramsPerM3;
	cell.totalEnergy += kilogramsPerM3 * ambientSpecificEnergy;
	const double volume = config.scale.cellVolumeM3();
	pendingSourceMassKg += kilogramsPerM3 * volume;
	pendingSourceEnergyJ += kilogramsPerM3 * ambientSpecificEnergy * volume;
	pendingEvent = true;
	transportActive = true;
}

void OmniAtmosphere::AddSpeciesMassDensity(
	std::size_t x,
	std::size_t y,
	std::size_t species,
	double kilogramsPerM3)
{
	if (x >= config.width || y >= config.height || species >= config.species.size() || !Finite(kilogramsPerM3))
		throw std::invalid_argument("invalid OmniAtmosphere species mass source");
	const auto index = Index(x, y);
	const auto primitive = Derive(index, state[index]);
	const double temperature = primitive.finite ? primitive.temperature : config.referenceTemperature;
	const double specificInternal = SpeciesCv(config.species[species]) * temperature +
		(species == OMNI_SPECIES_H2O ? OmniThermal::LatentHeatVaporizationJPerKg : 0.0);
	speciesState[SpeciesIndex(index, species)] += kilogramsPerM3;
	state[index].density += kilogramsPerM3;
	state[index].totalEnergy += kilogramsPerM3 * specificInternal;
	const double mass = kilogramsPerM3 * config.scale.cellVolumeM3();
	pendingSourceSpeciesMassKg[species] += mass;
	pendingSourceMassKg += mass;
	pendingSourceEnergyJ += kilogramsPerM3 * specificInternal * config.scale.cellVolumeM3();
	pendingEvent = true;
	transportActive = true;
	phaseActive = phaseActive || species == OMNI_SPECIES_H2O;
}

bool OmniAtmosphere::ApplyReactionSpeciesTransfer(
	std::size_t x,
	std::size_t y,
	const std::vector<double> &speciesMassDeltaKg,
	double chemicalEnergyJ,
	double parcelVelocityX,
	double parcelVelocityY,
	OmniAtmosphereReactionTransfer *result)
{
	OmniAtmosphereReactionTransfer transfer{};
	if (result)
		*result = transfer;
	if (x >= config.width || y >= config.height || speciesMassDeltaKg.size() != config.species.size() ||
		!Finite(chemicalEnergyJ) || !Finite(parcelVelocityX) || !Finite(parcelVelocityY))
		return false;
	for (const auto mass : speciesMassDeltaKg)
		if (!Finite(mass))
			return false;
	const auto cell = Index(x, y);
	if (blocked[cell])
		return false;
	const auto primitive = Derive(cell, state[cell]);
	if (!primitive.finite)
		return false;
	const double volume = config.scale.cellVolumeM3();
	if (!(volume > 0.0))
		return false;
	double totalMassDelta = 0.0;
	double sensibleEnergy = 0.0;
	std::vector<double> nextSpeciesMassDensity(config.species.size(), 0.0);
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		const double deltaDensity = speciesMassDeltaKg[species] / volume;
		const double nextDensity = speciesState[SpeciesIndex(cell, species)] + deltaDensity;
		if (!Finite(nextDensity) || nextDensity < 0.0)
			return false;
		nextSpeciesMassDensity[species] = nextDensity;
		totalMassDelta += speciesMassDeltaKg[species];
		sensibleEnergy += speciesMassDeltaKg[species] *
			(SpeciesCv(config.species[species]) * primitive.temperature +
				(species == OMNI_SPECIES_H2O ? OmniThermal::LatentHeatVaporizationJPerKg : 0.0));
	}
	const double nextDensity = state[cell].density + totalMassDelta / volume;
	if (!Finite(nextDensity) || nextDensity < config.densityFloor)
		return false;
	const double sourceMomentumX = totalMassDelta * parcelVelocityX;
	const double sourceMomentumY = totalMassDelta * parcelVelocityY;
	const double sourceKineticEnergy = 0.5 * totalMassDelta *
		(Square(parcelVelocityX) + Square(parcelVelocityY));
	const double totalEnergyDelta = sensibleEnergy + sourceKineticEnergy + chemicalEnergyJ;
	const double nextMomentumX = state[cell].momentumX + sourceMomentumX / volume;
	const double nextMomentumY = state[cell].momentumY + sourceMomentumY / volume;
	const double nextTotalEnergy = state[cell].totalEnergy + totalEnergyDelta / volume;
	const double nextKineticEnergy = 0.5 *
		(Square(nextMomentumX) + Square(nextMomentumY)) / nextDensity;
	double nextHeatCapacityDensity =
		condensedWaterDensity[cell] * OmniThermal::LiquidSpecificHeatJKgK;
	double nextGasConstantDensity = 0.0;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		nextHeatCapacityDensity += nextSpeciesMassDensity[species] * SpeciesCv(config.species[species]);
		nextGasConstantDensity += nextSpeciesMassDensity[species] * SpeciesGasConstant(config.species[species]);
	}
	const double nextLatentEnergyDensity =
		nextSpeciesMassDensity[OMNI_SPECIES_H2O] * OmniThermal::LatentHeatVaporizationJPerKg;
	const double minimumTemperatureK = std::max(
		1.0, config.pressureFloor / std::max(nextGasConstantDensity, 1.0e-300));
	const double requiredSensibleEnergyDensity = std::max(
		config.internalEnergyFloor, nextHeatCapacityDensity * minimumTemperatureK);
	const double requiredTotalEnergy =
		nextKineticEnergy + nextLatentEnergyDensity + requiredSensibleEnergyDensity;
	if (!Finite(sourceMomentumX) || !Finite(sourceMomentumY) || !Finite(totalEnergyDelta) ||
		!Finite(nextMomentumX) || !Finite(nextMomentumY) || !Finite(nextTotalEnergy) ||
		!Finite(nextKineticEnergy) || !Finite(nextHeatCapacityDensity) ||
		!(nextHeatCapacityDensity > 0.0) || !Finite(nextGasConstantDensity) ||
		!(nextGasConstantDensity > 0.0) || !Finite(nextLatentEnergyDensity) ||
		!Finite(requiredTotalEnergy) || nextTotalEnergy < requiredTotalEnergy)
		return false;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		const double deltaDensity = speciesMassDeltaKg[species] / volume;
		speciesState[SpeciesIndex(cell, species)] += deltaDensity;
		pendingSourceSpeciesMassKg[species] += speciesMassDeltaKg[species];
	}
	// Reaction transfers are also external water injections/withdrawals.  Keep
	// the phase-equilibrium pass armed when a transfer introduces H2O into a
	// previously dry atmosphere (the legacy AddSpeciesMassDensity path did
	// this implicitly).
	if (OMNI_SPECIES_H2O < nextSpeciesMassDensity.size() &&
		nextSpeciesMassDensity[OMNI_SPECIES_H2O] > 0.0)
		phaseActive = true;
	state[cell].density = nextDensity;
	state[cell].momentumX = nextMomentumX;
	state[cell].momentumY = nextMomentumY;
	state[cell].totalEnergy = nextTotalEnergy;
	pendingSourceMassKg += totalMassDelta;
	pendingSourceMomentumX += sourceMomentumX;
	pendingSourceMomentumY += sourceMomentumY;
	pendingSourceEnergyJ += totalEnergyDelta;
	pendingEvent = true;
	transportActive = true;
	transfer.committed = true;
	transfer.gasMassDeltaKg = totalMassDelta;
	transfer.sourceMomentumX = sourceMomentumX;
	transfer.sourceMomentumY = sourceMomentumY;
	transfer.sensibleEnergyDeltaJ = sensibleEnergy;
	transfer.sourceKineticEnergyJ = sourceKineticEnergy;
	transfer.chemicalEnergyJ = chemicalEnergyJ;
	transfer.totalEnergyDeltaJ = totalEnergyDelta;
	if (result)
		*result = transfer;
	return true;
}

void OmniAtmosphere::SetSpeciesMassFractions(
	std::size_t x,
	std::size_t y,
	const std::vector<double> &massFractions)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere species coordinate");
	const auto fractions = NormalizeFractions(massFractions, config.species.size());
	const auto index = Index(x, y);
	const double volume = config.scale.cellVolumeM3();
	const auto before = Derive(index, state[index]);
	const double oldEnergy = state[index].totalEnergy;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		auto &density = speciesState[SpeciesIndex(index, species)];
		const double nextDensity = state[index].density * fractions[species];
		pendingSourceSpeciesMassKg[species] += (nextDensity - density) * volume;
		density = nextDensity;
	}
	if (before.finite)
	{
		const double kinetic = 0.5 * (Square(state[index].momentumX) + Square(state[index].momentumY)) / state[index].density;
		double heatCapacityDensity = condensedWaterDensity[index] * OmniThermal::LiquidSpecificHeatJKgK;
		for (std::size_t species = 0; species < config.species.size(); ++species)
			heatCapacityDensity += speciesState[SpeciesIndex(index, species)] * SpeciesCv(config.species[species]);
		state[index].totalEnergy = kinetic + heatCapacityDensity * before.temperature +
			speciesState[SpeciesIndex(index, OMNI_SPECIES_H2O)] * OmniThermal::LatentHeatVaporizationJPerKg;
		pendingSourceEnergyJ += (state[index].totalEnergy - oldEnergy) * volume;
	}
	pendingEvent = true;
	transportActive = true;
	phaseActive = phaseActive || fractions[OMNI_SPECIES_H2O] > 0.0;
}

void OmniAtmosphere::SetCondensedWaterDensity(std::size_t x, std::size_t y, double kilogramsPerM3)
{
	if (x >= config.width || y >= config.height || !Finite(kilogramsPerM3) || kilogramsPerM3 < 0.0)
		throw std::invalid_argument("invalid OmniAtmosphere condensed water state");
	const auto index = Index(x, y);
	const auto primitive = Derive(index, state[index]);
	const double old = condensedWaterDensity[index];
	condensedWaterDensity[index] = kilogramsPerM3;
	const double temperature = primitive.finite ? primitive.temperature : config.referenceTemperature;
	const double energy = (kilogramsPerM3 - old) * OmniThermal::LiquidSpecificHeatJKgK * temperature;
	state[index].totalEnergy += energy;
	pendingSourceMassKg += (kilogramsPerM3 - old) * config.scale.cellVolumeM3();
	pendingSourceEnergyJ += energy * config.scale.cellVolumeM3();
	pendingEvent = true;
	transportActive = true;
	phaseActive = config.species.size() > OMNI_SPECIES_H2O &&
		(TotalCondensedWaterMassKg() > 0.0 || TotalSpeciesMassKg(OMNI_SPECIES_H2O) > 0.0);
}

bool OmniAtmosphere::RestoreSerializedCell(
	std::size_t x,
	std::size_t y,
	const std::vector<double> &speciesMassDensity,
	double momentumX,
	double momentumY,
	double totalEnergy,
	double condensedWaterMassDensity)
{
	if (x >= config.width || y >= config.height ||
		!OmniValidateSerializedAtmosphereCell(
			config, speciesMassDensity, momentumX, momentumY, totalEnergy,
			condensedWaterMassDensity, true))
	{
		return false;
	}
	double density = 0.0;
	for (double value : speciesMassDensity)
		density += value;
	const auto cell = Index(x, y);
	const auto oldState = state[cell];
	const double oldCondensed = condensedWaterDensity[cell];
	std::vector<double> oldSpecies(config.species.size(), 0.0);
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		oldSpecies[species] = speciesState[SpeciesIndex(cell, species)];
		speciesState[SpeciesIndex(cell, species)] = speciesMassDensity[species];
	}
	condensedWaterDensity[cell] = condensedWaterMassDensity;
	state[cell] = { density, momentumX, momentumY, totalEnergy };
	const double volume = config.scale.cellVolumeM3();
	pendingSourceMassKg +=
		((density + condensedWaterMassDensity) - (oldState.density + oldCondensed)) * volume;
	pendingSourceMomentumX += (momentumX - oldState.momentumX) * volume;
	pendingSourceMomentumY += (momentumY - oldState.momentumY) * volume;
	pendingSourceEnergyJ += (totalEnergy - oldState.totalEnergy) * volume;
	for (std::size_t species = 0; species < config.species.size(); ++species)
		pendingSourceSpeciesMassKg[species] += (speciesMassDensity[species] - oldSpecies[species]) * volume;
	pendingEvent = true;
	transportActive = true;
	phaseActive = phaseActive || condensedWaterMassDensity > 0.0 ||
		speciesMassDensity[OMNI_SPECIES_H2O] > 0.0;
	return true;
}

OmniAtmosphereRegionRestoreToken OmniAtmosphere::BeginRegionStateRestore() const
{
	return {
		pendingSourceMassKg,
		pendingSourceMomentumX,
		pendingSourceMomentumY,
		pendingSourceEnergyJ,
		pendingSourceSpeciesMassKg,
	};
}

void OmniAtmosphere::FinalizeRegionStateRestore(
	const OmniAtmosphereRegionRestoreToken &token,
	std::span<const std::size_t> restoredCells)
{
	if (restoredCells.empty())
		return;
	if (token.pendingSourceSpeciesMassKg.size() != pendingSourceSpeciesMassKg.size())
		throw std::invalid_argument("invalid OmniAtmosphere region restore token");
	for (const auto cell : restoredCells)
	{
		if (cell >= state.size())
			throw std::out_of_range("OmniAtmosphere region restore cell");
		next[cell] = state[cell];
		condensedWaterNext[cell] = condensedWaterDensity[cell];
		for (std::size_t species = 0; species < config.species.size(); ++species)
			speciesNext[SpeciesIndex(cell, species)] = speciesState[SpeciesIndex(cell, species)];
	}
	pendingSourceMassKg = token.pendingSourceMassKg;
	pendingSourceMomentumX = token.pendingSourceMomentumX;
	pendingSourceMomentumY = token.pendingSourceMomentumY;
	pendingSourceEnergyJ = token.pendingSourceEnergyJ;
	pendingSourceSpeciesMassKg = token.pendingSourceSpeciesMassKg;
	pendingEvent = true;
	compressibleActive = true;
	transportActive = true;
	phaseActive = false;
	for (double value : condensedWaterDensity)
		phaseActive = phaseActive || value > 0.0;
	for (std::size_t cell = 0; cell < state.size() && !phaseActive; ++cell)
		phaseActive = phaseActive || speciesState[SpeciesIndex(cell, OMNI_SPECIES_H2O)] > 0.0;
}

void OmniAtmosphere::FinalizeStateRestore()
{
	next = state;
	speciesNext = speciesState;
	condensedWaterNext = condensedWaterDensity;
	ledger = {};
	pendingSourceMassKg = 0.0;
	pendingSourceMomentumX = 0.0;
	pendingSourceMomentumY = 0.0;
	pendingSourceEnergyJ = 0.0;
	std::fill(pendingSourceSpeciesMassKg.begin(), pendingSourceSpeciesMassKg.end(), 0.0);
	pendingEvent = true;
	compressibleActive = true;
	transportActive = true;
	phaseActive = false;
	for (double value : condensedWaterDensity)
		phaseActive = phaseActive || value > 0.0;
	for (std::size_t cell = 0; cell < state.size() && !phaseActive; ++cell)
		phaseActive = phaseActive || speciesState[SpeciesIndex(cell, OMNI_SPECIES_H2O)] > 0.0;
}

void OmniAtmosphere::SetGravityY(double metresPerSecondSquared)
{
	if (!Finite(metresPerSecondSquared))
		throw std::invalid_argument("invalid OmniAtmosphere gravity");
	config.gravityY = metresPerSecondSquared;
}

void OmniAtmosphere::SetCell(std::size_t x, std::size_t y, OmniAtmosphereConservative value)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere state coordinate");
	// SetCell defines an initial/debug state. Its normalization is not a solver
	// correction and the resulting value becomes the next ledger's baseline.
	ApplyFloors(value, false);
	const auto index = Index(x, y);
	state[index] = value;
	NormalizeSpecies(index, value.density, false);
	pendingEvent = true;
	transportActive = true;
}

void OmniAtmosphere::ImportLegacyProjection(std::size_t x, std::size_t y, OmniAtmosphereConservative value)
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere Legacy projection coordinate");
	const auto index = Index(x, y);
	const auto old = state[index];
	auto close = [](double left, double right, double relative, double absolute) {
		return std::abs(left - right) <= absolute + relative * std::max(std::abs(left), std::abs(right));
	};
	if (close(old.density, value.density, 5.0e-6, 1.0e-12) &&
		close(old.momentumX, value.momentumX, 5.0e-6, 1.0e-9) &&
		close(old.momentumY, value.momentumY, 5.0e-6, 1.0e-9) &&
		close(old.totalEnergy, value.totalEnergy, 5.0e-6, 1.0e-6))
	{
		return;
	}
	// Legacy edits are external source normalization. The normalized delta below
	// is recorded as source mass/momentum/energy, not as a numerical solver clamp.
	ApplyFloors(value, false);
	const double volume = config.scale.cellVolumeM3();
	const double oldDensity = old.density;
	if (oldDensity > 0.0)
	{
		for (std::size_t species = 0; species < config.species.size(); ++species)
		{
			auto &density = speciesState[SpeciesIndex(index, species)];
			const double nextDensity = density * value.density / oldDensity;
			pendingSourceSpeciesMassKg[species] += (nextDensity - density) * volume;
			density = nextDensity;
		}
		// The per-channel rescale is exact only in infinite precision, so the
		// channels can re-sum one ULP below value.density. When ApplyFloors has
		// just clamped that density onto the floor, the sum lands below the floor
		// and OPS v3 rejects the cell on every later step -- nothing downstream
		// repairs it, because this projection runs outside the solver step. Close
		// the residual on the largest channel and book it as source mass, which
		// is this function's external-normalization contract.
		double speciesSum = 0.0;
		std::size_t largest = 0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
		{
			const double channel = speciesState[SpeciesIndex(index, species)];
			speciesSum += channel;
			if (channel > speciesState[SpeciesIndex(index, largest)])
				largest = species;
		}
		if (Finite(speciesSum) && speciesSum < value.density)
		{
			auto &channel = speciesState[SpeciesIndex(index, largest)];
			const double before = channel;
			const double corrected = channel + (value.density - speciesSum);
			if (Finite(corrected) && corrected >= 0.0)
				channel = corrected;
			for (int attempt = 0; attempt < 8 &&
				speciesSum + (channel - before) < value.density; ++attempt)
				channel = std::nextafter(
					channel, std::numeric_limits<double>::infinity());
			pendingSourceSpeciesMassKg[largest] += (channel - before) * volume;
		}
	}
	else
	{
		NormalizeSpecies(index, value.density, false);
	}
	const double kinetic = 0.5 * (Square(value.momentumX) + Square(value.momentumY)) / value.density;
	const double legacyInternal = std::max(value.totalEnergy - kinetic, config.internalEnergyFloor);
	const double legacyPressure = (config.gamma - 1.0) * legacyInternal;
	const double legacyTemperature = legacyPressure / (value.density * config.gasConstant);
	double heatCapacityDensity = condensedWaterDensity[index] * OmniThermal::LiquidSpecificHeatJKgK;
	for (std::size_t species = 0; species < config.species.size(); ++species)
		heatCapacityDensity += speciesState[SpeciesIndex(index, species)] * SpeciesCv(config.species[species]);
	value.totalEnergy = kinetic + heatCapacityDensity * legacyTemperature +
		speciesState[SpeciesIndex(index, OMNI_SPECIES_H2O)] * OmniThermal::LatentHeatVaporizationJPerKg;
	// Legacy projection is an external source path and runs again in AfterSim,
	// after particle updates.  A gamma-only legacy temperature can be below the
	// strict v3 mixture/latent-energy floor when the cell contains condensed
	// water; canonicalize the projected state before it can reach an OPS save or
	// the next heartbeat.  The resulting delta remains an external source in the
	// existing pending-source ledger below.
	double requiredTotalEnergy = 0.0;
	const auto speciesBegin = speciesState.data() + SpeciesIndex(index, 0);
	if (ComputeSerializedAtmosphereRequiredEnergy(
			config,
			std::span<const double>(speciesBegin, config.species.size()),
			value.momentumX, value.momentumY,
			condensedWaterDensity[index], requiredTotalEnergy) &&
		value.totalEnergy < requiredTotalEnergy)
	{
		value.totalEnergy = requiredTotalEnergy;
		for (int attempt = 0; attempt < 8 &&
			!OmniValidateSerializedAtmosphereCell(
				config,
				std::span<const double>(speciesBegin, config.species.size()),
				value.momentumX, value.momentumY,
				value.totalEnergy, condensedWaterDensity[index], true); ++attempt)
		{
			value.totalEnergy = std::nextafter(
				value.totalEnergy, std::numeric_limits<double>::infinity());
		}
	}
	pendingSourceMassKg += (value.density - old.density) * volume;
	pendingSourceMomentumX += (value.momentumX - old.momentumX) * volume;
	pendingSourceMomentumY += (value.momentumY - old.momentumY) * volume;
	pendingSourceEnergyJ += (value.totalEnergy - old.totalEnergy) * volume;
	state[index] = value;
	pendingEvent = true;
	transportActive = true;
	phaseActive = phaseActive || speciesState[SpeciesIndex(index, OMNI_SPECIES_H2O)] > 0.0;
}

const OmniAtmosphereSpeciesDefinition &OmniAtmosphere::SpeciesDefinition(std::size_t index) const
{
	if (index >= config.species.size())
		throw std::out_of_range("OmniAtmosphere species index");
	return config.species[index];
}

std::vector<double> OmniAtmosphere::AmbientSpeciesState() const
{
	std::vector<double> result(config.species.size(), 0.0);
	for (std::size_t species = 0; species < result.size(); ++species)
		result[species] = config.referenceDensity * config.referenceMassFractions[species];
	return result;
}

OmniAtmosphereConservative OmniAtmosphere::AmbientState() const
{
	const double internal = config.referenceDensity *
		MixtureCv(config.species, config.referenceMassFractions) * config.referenceTemperature +
		config.referenceDensity * config.referenceMassFractions[OMNI_SPECIES_H2O] *
		OmniThermal::LatentHeatVaporizationJPerKg;
	return {
		config.referenceDensity,
		0.0,
		0.0,
		internal,
	};
}

OmniAtmospherePrimitive OmniAtmosphere::Derive(
	std::size_t cell,
	const OmniAtmosphereConservative &value) const
{
	OmniAtmospherePrimitive primitive{};
	primitive.density = value.density;
	if (!Finite(value.density) || !Finite(value.momentumX) || !Finite(value.momentumY) ||
		!Finite(value.totalEnergy) || value.density <= 0.0)
		return primitive;

	primitive.velocityX = value.momentumX / value.density;
	primitive.velocityY = value.momentumY / value.density;
	const double kinetic = 0.5 * value.density * (Square(primitive.velocityX) + Square(primitive.velocityY));
	double heatCapacityDensity = cell < condensedWaterDensity.size()
		? condensedWaterDensity[cell] * OmniThermal::LiquidSpecificHeatJKgK
		: 0.0;
	double gasConstantDensity = 0.0;
	double cpDensity = 0.0;
	double waterDensity = 0.0;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		const double density = cell < state.size()
			? speciesState[SpeciesIndex(cell, species)]
			: config.referenceDensity * config.referenceMassFractions[species];
		heatCapacityDensity += density * SpeciesCv(config.species[species]);
		cpDensity += density * config.species[species].specificHeatCpJKgK;
		gasConstantDensity += density * SpeciesGasConstant(config.species[species]);
		if (species == OMNI_SPECIES_H2O)
			waterDensity = density;
	}
	if (!(heatCapacityDensity > 0.0) || !(gasConstantDensity > 0.0))
		return primitive;
	const double internal = value.totalEnergy - kinetic - waterDensity * OmniThermal::LatentHeatVaporizationJPerKg;
	primitive.temperature = internal / heatCapacityDensity;
	primitive.pressure = primitive.temperature * gasConstantDensity;
	primitive.mixtureGasConstant = gasConstantDensity / value.density;
	primitive.mixtureGamma = cpDensity / heatCapacityDensity;
	primitive.soundSpeed = std::sqrt(primitive.mixtureGamma * primitive.pressure / value.density);
	primitive.waterPartialPressurePa = waterDensity * SpeciesGasConstant(config.species[OMNI_SPECIES_H2O]) * primitive.temperature;
	primitive.saturationPressurePa = OmniThermal::SaturationPressurePa(primitive.temperature);
	primitive.relativeHumidity = OmniThermal::RelativeHumidity(primitive.waterPartialPressurePa, primitive.temperature);
	primitive.condensedWaterDensity = cell < condensedWaterDensity.size() ? condensedWaterDensity[cell] : 0.0;
	primitive.finite = Finite(primitive.velocityX) && Finite(primitive.velocityY) &&
		Finite(primitive.temperature) && primitive.temperature > 0.0 &&
		Finite(primitive.pressure) && primitive.pressure > 0.0 && Finite(primitive.soundSpeed);
	return primitive;
}

OmniAtmospherePrimitive OmniAtmosphere::Derive(const OmniAtmosphereConservative &value) const
{
	OmniAtmospherePrimitive primitive{};
	primitive.density = value.density;
	if (!Finite(value.density) || !Finite(value.momentumX) || !Finite(value.momentumY) ||
		!Finite(value.totalEnergy) || value.density <= 0.0)
	{
		return primitive;
	}
	primitive.velocityX = value.momentumX / value.density;
	primitive.velocityY = value.momentumY / value.density;
	const double kinetic = 0.5 * value.density * (Square(primitive.velocityX) + Square(primitive.velocityY));
	const double internal = value.totalEnergy - kinetic;
	primitive.pressure = (config.gamma - 1.0) * internal;
	if (!Finite(primitive.velocityX) || !Finite(primitive.velocityY) ||
		!Finite(primitive.pressure) || primitive.pressure <= 0.0)
	{
		return primitive;
	}
	primitive.temperature = primitive.pressure / (value.density * config.gasConstant);
	primitive.soundSpeed = std::sqrt(config.gamma * primitive.pressure / value.density);
	primitive.finite = Finite(primitive.temperature) && primitive.temperature > 0.0 && Finite(primitive.soundSpeed);
	return primitive;
}

OmniAtmosphere::Flux OmniAtmosphere::PhysicalFlux(const OmniAtmosphereConservative &value, bool xDirection) const
{
	const auto primitive = Derive(value);
	if (!primitive.finite)
		return {};
	if (xDirection)
	{
		return {
			value.momentumX,
			value.momentumX * primitive.velocityX + primitive.pressure,
			value.momentumY * primitive.velocityX,
			(value.totalEnergy + primitive.pressure) * primitive.velocityX,
		};
	}
	return {
		value.momentumY,
		value.momentumX * primitive.velocityY,
		value.momentumY * primitive.velocityY + primitive.pressure,
		(value.totalEnergy + primitive.pressure) * primitive.velocityY,
	};
}

OmniAtmosphere::Flux OmniAtmosphere::PhysicalFlux(
	std::size_t cell,
	const OmniAtmosphereConservative &value,
	bool xDirection) const
{
	const auto primitive = Derive(cell, value);
	if (!primitive.finite)
		return {};
	if (xDirection)
	{
		return {
			value.momentumX,
			value.momentumX * primitive.velocityX + primitive.pressure,
			value.momentumY * primitive.velocityX,
			(value.totalEnergy + primitive.pressure) * primitive.velocityX,
		};
	}
	return {
		value.momentumY,
		value.momentumX * primitive.velocityY,
		value.momentumY * primitive.velocityY + primitive.pressure,
		(value.totalEnergy + primitive.pressure) * primitive.velocityY,
	};
}

OmniAtmosphere::Flux OmniAtmosphere::RusanovFlux(
	const OmniAtmosphereConservative &left,
	const OmniAtmosphereConservative &right,
	bool xDirection,
	bool acoustic) const
{
	const auto leftPrimitive = Derive(left);
	const auto rightPrimitive = Derive(right);
	if (!leftPrimitive.finite || !rightPrimitive.finite)
		return {};
	const auto leftFlux = PhysicalFlux(left, xDirection);
	const auto rightFlux = PhysicalFlux(right, xDirection);
	const double leftNormalVelocity = xDirection ? leftPrimitive.velocityX : leftPrimitive.velocityY;
	const double rightNormalVelocity = xDirection ? rightPrimitive.velocityX : rightPrimitive.velocityY;
	const double leftSignal = std::abs(leftNormalVelocity) + (acoustic ? leftPrimitive.soundSpeed : 0.0);
	const double rightSignal = std::abs(rightNormalVelocity) + (acoustic ? rightPrimitive.soundSpeed : 0.0);
	const double signal = std::max(leftSignal, rightSignal);
	return {
		0.5 * (leftFlux.density + rightFlux.density) - 0.5 * signal * (right.density - left.density),
		0.5 * (leftFlux.momentumX + rightFlux.momentumX) - 0.5 * signal * (right.momentumX - left.momentumX),
		0.5 * (leftFlux.momentumY + rightFlux.momentumY) - 0.5 * signal * (right.momentumY - left.momentumY),
		0.5 * (leftFlux.totalEnergy + rightFlux.totalEnergy) - 0.5 * signal * (right.totalEnergy - left.totalEnergy),
	};
}

OmniAtmosphere::Flux OmniAtmosphere::RusanovFlux(
	std::size_t leftCell,
	std::size_t rightCell,
	const OmniAtmosphereConservative &left,
	const OmniAtmosphereConservative &right,
	bool xDirection,
	bool acoustic) const
{
	const auto leftPrimitive = Derive(leftCell, left);
	const auto rightPrimitive = Derive(rightCell, right);
	if (!leftPrimitive.finite || !rightPrimitive.finite)
		return {};
	const auto leftFlux = PhysicalFlux(leftCell, left, xDirection);
	const auto rightFlux = PhysicalFlux(rightCell, right, xDirection);
	const double leftNormalVelocity = xDirection ? leftPrimitive.velocityX : leftPrimitive.velocityY;
	const double rightNormalVelocity = xDirection ? rightPrimitive.velocityX : rightPrimitive.velocityY;
	const double leftSignal = std::abs(leftNormalVelocity) + (acoustic ? leftPrimitive.soundSpeed : 0.0);
	const double rightSignal = std::abs(rightNormalVelocity) + (acoustic ? rightPrimitive.soundSpeed : 0.0);
	const double signal = std::max(leftSignal, rightSignal);
	return {
		0.5 * (leftFlux.density + rightFlux.density) - 0.5 * signal * (right.density - left.density),
		0.5 * (leftFlux.momentumX + rightFlux.momentumX) - 0.5 * signal * (right.momentumX - left.momentumX),
		0.5 * (leftFlux.momentumY + rightFlux.momentumY) - 0.5 * signal * (right.momentumY - left.momentumY),
		0.5 * (leftFlux.totalEnergy + rightFlux.totalEnergy) - 0.5 * signal * (right.totalEnergy - left.totalEnergy),
	};
}

void OmniAtmosphere::RecordBoundaryFlux(const Flux &flux, bool, bool positiveOutward, double dt)
{
	const double orientation = positiveOutward ? 1.0 : -1.0;
	const double mass = orientation * flux.density * dt * config.scale.faceAreaM2();
	const double energy = orientation * flux.totalEnergy * dt * config.scale.faceAreaM2();
	ledger.boundaryMomentumXOut += orientation * flux.momentumX * dt * config.scale.faceAreaM2();
	ledger.boundaryMomentumYOut += orientation * flux.momentumY * dt * config.scale.faceAreaM2();
	if (mass >= 0.0)
		ledger.boundaryMassOutKg += mass;
	else
		ledger.boundaryMassInKg -= mass;
	if (energy >= 0.0)
		ledger.boundaryEnergyOutJ += energy;
	else
		ledger.boundaryEnergyInJ -= energy;
}

bool OmniAtmosphere::CompressibleFeaturesPresent() const
{
	auto differs = [](double left, double right, double relative, double absolute) {
		return std::abs(left - right) > absolute + relative * std::max(std::abs(left), std::abs(right));
	};
	for (std::size_t y = 0; y < config.height; ++y)
	{
		for (std::size_t x = 0; x < config.width; ++x)
		{
			const auto index = Index(x, y);
			if (blocked[index])
				continue;
			const auto current = Derive(index, state[index]);
			if (!current.finite)
				return true;
			const double wallVelocityTolerance = 1.0e-6;
			if (config.boundary == OmniAtmosphereBoundary::Sealed &&
				(((x == 0 || x + 1 == config.width) && std::abs(current.velocityX) > wallVelocityTolerance) ||
				 ((y == 0 || y + 1 == config.height) && std::abs(current.velocityY) > wallVelocityTolerance)))
			{
				return true;
			}
			auto compare = [&](std::size_t nx, std::size_t ny) {
				const auto neighbourIndex = Index(nx, ny);
				if (blocked[neighbourIndex])
				{
					const bool xFace = nx != x;
					const double normalVelocity = xFace ? current.velocityX : current.velocityY;
					return std::abs(normalVelocity) > wallVelocityTolerance;
				}
				const auto neighbour = Derive(neighbourIndex, state[neighbourIndex]);
				return !neighbour.finite ||
					differs(current.pressure, neighbour.pressure, 1.0e-8, 1.0e-5) ||
					differs(current.velocityX, neighbour.velocityX, 1.0e-8, 1.0e-6) ||
					differs(current.velocityY, neighbour.velocityY, 1.0e-8, 1.0e-6);
			};
			if (x + 1 < config.width && compare(x + 1, y))
				return true;
			if (y + 1 < config.height && compare(x, y + 1))
				return true;
		}
	}
	return false;
}

void OmniAtmosphere::AdvanceOnce(double dt, bool acoustic)
{
	const std::size_t width = config.width;
	const std::size_t height = config.height;
	std::vector<Flux> fluxX((width + 1) * height);
	std::vector<Flux> fluxY(width * (height + 1));
	std::vector<double> speciesFluxX((width + 1) * height * config.species.size(), 0.0);
	std::vector<double> speciesFluxY(width * (height + 1) * config.species.size(), 0.0);
	const auto ambient = AmbientState();
	auto reflect = [](OmniAtmosphereConservative value, bool xDirection) {
		if (xDirection)
			value.momentumX = -value.momentumX;
		else
			value.momentumY = -value.momentumY;
		return value;
	};

	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t faceX = 0; faceX <= width; ++faceX)
		{
			Flux flux{};
			if (faceX == 0 || faceX == width)
			{
				if (config.boundary == OmniAtmosphereBoundary::Periodic)
				{
					const bool leftBlocked = blocked[Index(width - 1, y)] != 0;
					const bool rightBlocked = blocked[Index(0, y)] != 0;
					if (!leftBlocked && !rightBlocked)
					{
						flux = RusanovFlux(Index(width - 1, y), Index(0, y), state[Index(width - 1, y)], state[Index(0, y)], true, acoustic);
					}
					else if (leftBlocked != rightBlocked)
					{
						// A periodic seam is still a physical face when one side is a
						// blocked cell. Mirror the fluid state exactly as for an internal
						// wall; a zero flux would create a one-sided pressure impulse.
						if (leftBlocked)
						{
							flux = RusanovFlux(Index(0, y), Index(0, y), reflect(state[Index(0, y)], true), state[Index(0, y)], true, acoustic);
							if (faceX == 0)
								RecordBoundaryFlux(flux, true, false, dt);
						}
						else
						{
							flux = RusanovFlux(Index(width - 1, y), Index(width - 1, y), state[Index(width - 1, y)], reflect(state[Index(width - 1, y)], true), true, acoustic);
							if (faceX == 0)
								RecordBoundaryFlux(flux, true, true, dt);
						}
					}
				}
				else if (config.boundary == OmniAtmosphereBoundary::Open)
				{
					const auto cellX = faceX == 0 ? 0U : width - 1;
					if (!blocked[Index(cellX, y)])
					{
						flux = faceX == 0
							? RusanovFlux(state.size(), Index(0, y), ambient, state[Index(0, y)], true, acoustic)
							: RusanovFlux(Index(width - 1, y), state.size(), state[Index(width - 1, y)], ambient, true, acoustic);
						RecordBoundaryFlux(flux, true, faceX == width, dt);
					}
				}
				else
				{
					const auto cellX = faceX == 0 ? 0U : width - 1;
					if (!blocked[Index(cellX, y)])
					{
						const auto &cell = state[Index(cellX, y)];
						flux = faceX == 0
							? RusanovFlux(Index(cellX, y), Index(cellX, y), reflect(cell, true), cell, true, acoustic)
							: RusanovFlux(Index(cellX, y), Index(cellX, y), cell, reflect(cell, true), true, acoustic);
						RecordBoundaryFlux(flux, true, faceX == width, dt);
					}
				}
			}
			else if (!blocked[Index(faceX - 1, y)] && !blocked[Index(faceX, y)])
			{
				flux = RusanovFlux(Index(faceX - 1, y), Index(faceX, y), state[Index(faceX - 1, y)], state[Index(faceX, y)], true, acoustic);
			}
			else if (!blocked[Index(faceX - 1, y)])
			{
				const auto &cell = state[Index(faceX - 1, y)];
				flux = RusanovFlux(Index(faceX - 1, y), Index(faceX - 1, y), cell, reflect(cell, true), true, acoustic);
				RecordBoundaryFlux(flux, true, true, dt);
			}
			else if (!blocked[Index(faceX, y)])
			{
				const auto &cell = state[Index(faceX, y)];
				flux = RusanovFlux(Index(faceX, y), Index(faceX, y), reflect(cell, true), cell, true, acoustic);
				RecordBoundaryFlux(flux, true, false, dt);
			}
			fluxX[y * (width + 1) + faceX] = flux;
		}
	}

	for (std::size_t faceY = 0; faceY <= height; ++faceY)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			Flux flux{};
			if (faceY == 0 || faceY == height)
			{
				if (config.boundary == OmniAtmosphereBoundary::Periodic)
				{
					const bool topBlocked = blocked[Index(x, height - 1)] != 0;
					const bool bottomBlocked = blocked[Index(x, 0)] != 0;
					if (!topBlocked && !bottomBlocked)
					{
						flux = RusanovFlux(Index(x, height - 1), Index(x, 0), state[Index(x, height - 1)], state[Index(x, 0)], false, acoustic);
					}
					else if (topBlocked != bottomBlocked)
					{
						if (topBlocked)
						{
							flux = RusanovFlux(Index(x, 0), Index(x, 0), reflect(state[Index(x, 0)], false), state[Index(x, 0)], false, acoustic);
							if (faceY == 0)
								RecordBoundaryFlux(flux, false, false, dt);
						}
						else
						{
							flux = RusanovFlux(Index(x, height - 1), Index(x, height - 1), state[Index(x, height - 1)], reflect(state[Index(x, height - 1)], false), false, acoustic);
							if (faceY == 0)
								RecordBoundaryFlux(flux, false, true, dt);
						}
					}
				}
				else if (config.boundary == OmniAtmosphereBoundary::Open)
				{
					const auto cellY = faceY == 0 ? 0U : height - 1;
					if (!blocked[Index(x, cellY)])
					{
						flux = faceY == 0
							? RusanovFlux(state.size(), Index(x, 0), ambient, state[Index(x, 0)], false, acoustic)
							: RusanovFlux(Index(x, height - 1), state.size(), state[Index(x, height - 1)], ambient, false, acoustic);
						RecordBoundaryFlux(flux, false, faceY == height, dt);
					}
				}
				else
				{
					const auto cellY = faceY == 0 ? 0U : height - 1;
					if (!blocked[Index(x, cellY)])
					{
						const auto &cell = state[Index(x, cellY)];
						flux = faceY == 0
							? RusanovFlux(Index(x, cellY), Index(x, cellY), reflect(cell, false), cell, false, acoustic)
							: RusanovFlux(Index(x, cellY), Index(x, cellY), cell, reflect(cell, false), false, acoustic);
						RecordBoundaryFlux(flux, false, faceY == height, dt);
					}
				}
			}
			else if (!blocked[Index(x, faceY - 1)] && !blocked[Index(x, faceY)])
			{
				flux = RusanovFlux(Index(x, faceY - 1), Index(x, faceY), state[Index(x, faceY - 1)], state[Index(x, faceY)], false, acoustic);
			}
			else if (!blocked[Index(x, faceY - 1)])
			{
				const auto &cell = state[Index(x, faceY - 1)];
				flux = RusanovFlux(Index(x, faceY - 1), Index(x, faceY - 1), cell, reflect(cell, false), false, acoustic);
				RecordBoundaryFlux(flux, false, true, dt);
			}
			else if (!blocked[Index(x, faceY)])
			{
				const auto &cell = state[Index(x, faceY)];
				flux = RusanovFlux(Index(x, faceY), Index(x, faceY), reflect(cell, false), cell, false, acoustic);
				RecordBoundaryFlux(flux, false, false, dt);
			}
			fluxY[faceY * width + x] = flux;
		}
	}

	auto speciesFraction = [&](std::size_t cell, std::size_t species) {
		if (cell >= state.size())
			return config.referenceMassFractions[species];
		const double density = state[cell].density;
		return density > 0.0 ? speciesState[SpeciesIndex(cell, species)] / density : 0.0;
	};
	auto fillSpeciesFlux = [&](double *target, double massFlux, std::size_t leftCell, std::size_t rightCell) {
		const std::size_t upwind = massFlux >= 0.0 ? leftCell : rightCell;
		for (std::size_t species = 0; species < config.species.size(); ++species)
			target[species] = massFlux * speciesFraction(upwind, species);
	};

	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t faceX = 0; faceX <= width; ++faceX)
		{
			std::size_t leftCell = state.size();
			std::size_t rightCell = state.size();
			bool active = true;
			if (faceX == 0 || faceX == width)
			{
				if (config.boundary == OmniAtmosphereBoundary::Periodic)
				{
					leftCell = Index(width - 1, y);
					rightCell = Index(0, y);
					if (blocked[leftCell] && blocked[rightCell])
						active = false;
					else if (blocked[leftCell])
						leftCell = rightCell;
					else if (blocked[rightCell])
						rightCell = leftCell;
				}
				else
				{
					const auto cell = Index(faceX == 0 ? 0U : width - 1, y);
					if (blocked[cell])
						active = false;
					else if (config.boundary == OmniAtmosphereBoundary::Open)
					{
						leftCell = faceX == 0 ? state.size() : cell;
						rightCell = faceX == 0 ? cell : state.size();
					}
					else
					{
						leftCell = rightCell = cell;
					}
				}
			}
			else
			{
				leftCell = Index(faceX - 1, y);
				rightCell = Index(faceX, y);
				if (blocked[leftCell] && blocked[rightCell])
					active = false;
				else if (blocked[leftCell])
					leftCell = rightCell;
				else if (blocked[rightCell])
					rightCell = leftCell;
			}
			if (active)
			{
				auto *target = &speciesFluxX[(y * (width + 1) + faceX) * config.species.size()];
				fillSpeciesFlux(target, fluxX[y * (width + 1) + faceX].density, leftCell, rightCell);
				if (config.boundary == OmniAtmosphereBoundary::Open && (faceX == 0 || faceX == width))
					RecordBoundarySpeciesFlux(std::vector<double>(target, target + config.species.size()), faceX == width, dt);
			}
		}
	}

	for (std::size_t faceY = 0; faceY <= height; ++faceY)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			std::size_t topCell = state.size();
			std::size_t bottomCell = state.size();
			bool active = true;
			if (faceY == 0 || faceY == height)
			{
				if (config.boundary == OmniAtmosphereBoundary::Periodic)
				{
					topCell = Index(x, height - 1);
					bottomCell = Index(x, 0);
					if (blocked[topCell] && blocked[bottomCell])
						active = false;
					else if (blocked[topCell])
						topCell = bottomCell;
					else if (blocked[bottomCell])
						bottomCell = topCell;
				}
				else
				{
					const auto cell = Index(x, faceY == 0 ? 0U : height - 1);
					if (blocked[cell])
						active = false;
					else if (config.boundary == OmniAtmosphereBoundary::Open)
					{
						topCell = faceY == 0 ? state.size() : cell;
						bottomCell = faceY == 0 ? cell : state.size();
					}
					else
					{
						topCell = bottomCell = cell;
					}
				}
			}
			else
			{
				topCell = Index(x, faceY - 1);
				bottomCell = Index(x, faceY);
				if (blocked[topCell] && blocked[bottomCell])
					active = false;
				else if (blocked[topCell])
					topCell = bottomCell;
				else if (blocked[bottomCell])
					bottomCell = topCell;
			}
			if (active)
			{
				auto *target = &speciesFluxY[(faceY * width + x) * config.species.size()];
				fillSpeciesFlux(target, fluxY[faceY * width + x].density, topCell, bottomCell);
				if (config.boundary == OmniAtmosphereBoundary::Open && (faceY == 0 || faceY == height))
					RecordBoundarySpeciesFlux(std::vector<double>(target, target + config.species.size()), faceY == height, dt);
			}
		}
	}

	const double factor = dt / config.scale.cellLengthM;
	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			const auto index = Index(x, y);
			if (blocked[index])
			{
				next[index] = state[index];
				for (std::size_t species = 0; species < config.species.size(); ++species)
					speciesNext[SpeciesIndex(index, species)] = speciesState[SpeciesIndex(index, species)];
				continue;
			}
			const auto &left = fluxX[y * (width + 1) + x];
			const auto &right = fluxX[y * (width + 1) + x + 1];
			const auto &top = fluxY[y * width + x];
			const auto &bottom = fluxY[(y + 1) * width + x];
			auto value = state[index];
			value.momentumX -= factor * ((right.momentumX - left.momentumX) + (bottom.momentumX - top.momentumX));
			value.momentumY -= factor * ((right.momentumY - left.momentumY) + (bottom.momentumY - top.momentumY));
			value.totalEnergy -= factor * ((right.totalEnergy - left.totalEnergy) + (bottom.totalEnergy - top.totalEnergy));
			double speciesSum = 0.0;
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double speciesLeft = speciesFluxX[(y * (width + 1) + x) * config.species.size() + species];
				const double speciesRight = speciesFluxX[(y * (width + 1) + x + 1) * config.species.size() + species];
				const double speciesTop = speciesFluxY[(y * width + x) * config.species.size() + species];
				const double speciesBottom = speciesFluxY[((y + 1) * width + x) * config.species.size() + species];
				double density = speciesState[SpeciesIndex(index, species)] -
					factor * ((speciesRight - speciesLeft) + (speciesBottom - speciesTop));
				if (!Finite(density) || density < 0.0)
				{
					const double old = density;
					density = 0.0;
					if (species < ledger.numericalSpeciesCorrectionKg.size() && Finite(old))
						ledger.numericalSpeciesCorrectionKg[species] += (density - old) * config.scale.cellVolumeM3();
				}
				speciesNext[SpeciesIndex(index, species)] = density;
				speciesSum += density;
			}
			value.density = speciesSum;
			ApplyFloors(value);
			if (value.density != speciesSum)
				RescaleSpeciesToDensity(speciesNext, index, value.density);
			next[index] = value;
		}
	}
	state.swap(next);
	speciesState.swap(speciesNext);
}

void OmniAtmosphere::ApplyFloors(OmniAtmosphereConservative &value, bool recordCorrection)
{
	const double oldDensity = value.density;
	const double oldMomentumX = value.momentumX;
	const double oldMomentumY = value.momentumY;
	const double oldEnergy = value.totalEnergy;
	if (!Finite(value.density) || !Finite(value.momentumX) || !Finite(value.momentumY) || !Finite(value.totalEnergy))
	{
		if (recordCorrection)
			ledger.nonFiniteCells++;
		value = AmbientState();
	}
	if (value.density < config.densityFloor)
	{
		value.density = config.densityFloor;
		value.momentumX = 0.0;
		value.momentumY = 0.0;
		if (recordCorrection)
			ledger.densityFloorHits++;
	}
	const double kinetic = 0.5 * (Square(value.momentumX) + Square(value.momentumY)) / value.density;
	const double pressureRequiredInternal = config.pressureFloor / (config.gamma - 1.0);
	const double oldInternal = value.totalEnergy - kinetic;
	const double requiredInternal = std::max(config.internalEnergyFloor, pressureRequiredInternal);
	if (oldInternal < requiredInternal)
	{
		value.totalEnergy = kinetic + requiredInternal;
		if (recordCorrection)
		{
			ledger.energyFloorHits++;
			if (oldInternal < pressureRequiredInternal)
				ledger.pressureFloorHits++;
		}
	}
	if (!recordCorrection)
		return;
	const double volume = config.scale.cellVolumeM3();
	if (Finite(oldDensity))
		ledger.numericalMassCorrectionKg += (value.density - oldDensity) * volume;
	if (Finite(oldMomentumX))
		ledger.numericalMomentumXCorrection += (value.momentumX - oldMomentumX) * volume;
	if (Finite(oldMomentumY))
		ledger.numericalMomentumYCorrection += (value.momentumY - oldMomentumY) * volume;
	if (Finite(oldEnergy))
		ledger.numericalEnergyCorrectionJ += (value.totalEnergy - oldEnergy) * volume;
}

void OmniAtmosphere::NormalizeSpecies(std::size_t cell, double targetDensity, bool recordCorrection)
{
	if (cell >= state.size() || !Finite(targetDensity) || targetDensity < 0.0)
		throw std::invalid_argument("invalid OmniAtmosphere species normalization");
	RescaleSpeciesToDensity(speciesState, cell, targetDensity, recordCorrection);
}

void OmniAtmosphere::RescaleSpeciesToDensity(
	std::vector<double> &speciesArray,
	std::size_t cell,
	double targetDensity,
	bool recordCorrection)
{
	double sum = 0.0;
	for (std::size_t species = 0; species < config.species.size(); ++species)
		sum += std::max(0.0, speciesArray[SpeciesIndex(cell, species)]);
	std::size_t largest = 0;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		auto &density = speciesArray[SpeciesIndex(cell, species)];
		const double old = density;
		density = sum > 0.0
			? std::max(0.0, density) * targetDensity / sum
			: targetDensity * config.referenceMassFractions[species];
		if (recordCorrection && species < ledger.numericalSpeciesCorrectionKg.size())
			ledger.numericalSpeciesCorrectionKg[species] += (density - old) * config.scale.cellVolumeM3();
		if (density > speciesArray[SpeciesIndex(cell, largest)])
			largest = species;
	}
	// The rescale above is exact only in infinite precision: re-summing the
	// scaled channels can land one ULP below targetDensity.  A cell clamped to
	// the density floor then fails the OPS v3 serializable predicate on every
	// later step, which stalls long runs instead of reporting a physical fault.
	// Correct the deterministic residual on the largest channel and close any
	// remaining roundoff, matching the guarantee the legacy v2 migration already
	// applies at the serialization boundary.
	if (!(targetDensity > 0.0))
		return;
	const auto currentSum = [&]() {
		double total = 0.0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
			total += speciesArray[SpeciesIndex(cell, species)];
		return total;
	};
	auto &correction = speciesArray[SpeciesIndex(cell, largest)];
	const double before = correction;
	const double corrected = correction + (targetDensity - currentSum());
	if (Finite(corrected) && corrected >= 0.0)
		correction = corrected;
	for (int attempt = 0; attempt < 8 && currentSum() < targetDensity; ++attempt)
		correction = std::nextafter(correction, std::numeric_limits<double>::infinity());
	if (recordCorrection && largest < ledger.numericalSpeciesCorrectionKg.size())
		ledger.numericalSpeciesCorrectionKg[largest] += (correction - before) * config.scale.cellVolumeM3();
}

void OmniAtmosphere::RepairSpeciesFloorOnDryChannel(std::size_t cell, double knownSum)
{
	if (cell >= state.size() || !(config.densityFloor > 0.0))
		return;
	if (!Finite(knownSum) || knownSum >= config.densityFloor)
		return;
	if (!(state[cell].density >= config.densityFloor))
		return;
	// Bound the repair to the same eight-step roundoff window the serialization
	// boundary already allows. A larger shortfall is genuine state loss: leave it
	// invalid so the runtime heartbeat and the OPS gate still report it.
	double repairableBound = config.densityFloor;
	for (int step = 0; step < 8; ++step)
		repairableBound = std::nextafter(repairableBound, 0.0);
	if (!(knownSum >= repairableBound))
		return;
	std::size_t target = config.species.size();
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		if (species == OMNI_SPECIES_H2O)
			continue;
		if (target == config.species.size() ||
			speciesState[SpeciesIndex(cell, species)] >
				speciesState[SpeciesIndex(cell, target)])
			target = species;
	}
	if (target == config.species.size())
		return;
	auto &channel = speciesState[SpeciesIndex(cell, target)];
	const double before = channel;
	const double corrected = channel + (config.densityFloor - knownSum);
	if (Finite(corrected) && corrected >= 0.0)
		channel = corrected;
	for (int attempt = 0; attempt < 8 &&
		knownSum + (channel - before) < config.densityFloor; ++attempt)
		channel = std::nextafter(channel, std::numeric_limits<double>::infinity());
	if (target < ledger.numericalSpeciesCorrectionKg.size())
		ledger.numericalSpeciesCorrectionKg[target] +=
			(channel - before) * config.scale.cellVolumeM3();
}

void OmniAtmosphere::ReportFirstFloorInvariantViolation(const char *pass)
{
	static int reported = 0;
	if (reported >= 5 || !(config.densityFloor > 0.0))
		return;
	for (std::size_t cell = 0; cell < state.size(); ++cell)
	{
		if (!(state[cell].density >= config.densityFloor))
			continue;
		double sum = 0.0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
			sum += speciesState[SpeciesIndex(cell, species)];
		if (!Finite(sum) || sum >= config.densityFloor)
			continue;
		++reported;
		std::fprintf(stderr,
			"FLOORDIAG pass=%s cell=%zu x=%zu y=%zu blocked=%d "
			"density=%.17g species_sum=%.17g floor=%.17g",
			pass, cell, cell % config.width, cell / config.width,
			int(blocked[cell]), state[cell].density, sum, config.densityFloor);
		for (std::size_t species = 0; species < config.species.size(); ++species)
			std::fprintf(stderr, " s%zu=%.17g", species,
				speciesState[SpeciesIndex(cell, species)]);
		std::fprintf(stderr, "\n");
		std::fflush(stderr);
		return;
	}
}

double OmniAtmosphere::SpeciesFlux(
	std::size_t species,
	std::size_t leftCell,
	std::size_t rightCell,
	const OmniAtmosphereConservative &left,
	const OmniAtmosphereConservative &right,
	bool xDirection,
	bool acoustic) const
{
	const auto flux = RusanovFlux(leftCell, rightCell, left, right, xDirection, acoustic);
	const auto sourceCell = flux.density >= 0.0 ? leftCell : rightCell;
	const double fraction = sourceCell < state.size()
		? SpeciesMassFraction(sourceCell % config.width, sourceCell / config.width, species)
		: config.referenceMassFractions[species];
	return flux.density * fraction;
}

void OmniAtmosphere::RecordBoundarySpeciesFlux(
	const std::vector<double> &flux,
	bool positiveOutward,
	double dt)
{
	const double orientation = positiveOutward ? 1.0 : -1.0;
	for (std::size_t species = 0; species < config.species.size() && species < flux.size(); ++species)
	{
		const double mass = orientation * flux[species] * dt * config.scale.faceAreaM2();
		if (mass >= 0.0)
			ledger.boundarySpeciesOutKg[species] += mass;
		else
			ledger.boundarySpeciesInKg[species] -= mass;
	}
}

double OmniAtmosphere::TransportStableTimestep(bool assumeGradient) const
{
	if ((!transportActive && !assumeGradient) ||
		(!config.speciesDiffusion && !config.thermalConduction))
		return std::numeric_limits<double>::infinity();
	const unsigned dimensions = (config.width > 1 ? 1U : 0U) +
		(config.height > 1 ? 1U : 0U);
	if (!dimensions)
		return std::numeric_limits<double>::infinity();

	bool speciesGradient = assumeGradient && config.speciesDiffusion;
	bool thermalGradient = assumeGradient && config.thermalConduction;
	bool invalidState = false;
	auto inspectFace = [&](std::size_t left, std::size_t right) {
		if (left == right || blocked[left] || blocked[right])
			return;
		const auto leftPrimitive = Derive(left, state[left]);
		const auto rightPrimitive = Derive(right, state[right]);
		if (!leftPrimitive.finite || !rightPrimitive.finite)
		{
			invalidState = true;
			return;
		}
		thermalGradient = thermalGradient ||
			std::abs(leftPrimitive.temperature - rightPrimitive.temperature) > 1.0e-9;
		if (!speciesGradient)
		{
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double leftFraction =
					speciesState[SpeciesIndex(left, species)] / state[left].density;
				const double rightFraction =
					speciesState[SpeciesIndex(right, species)] / state[right].density;
				if (!Finite(leftFraction) || !Finite(rightFraction))
				{
					invalidState = true;
					return;
				}
				if (std::abs(leftFraction - rightFraction) > 1.0e-12)
				{
					speciesGradient = true;
					break;
				}
			}
		}
	};
	for (std::size_t y = 0; y < config.height; ++y)
		for (std::size_t x = 1; x < config.width; ++x)
			inspectFace(Index(x - 1, y), Index(x, y));
	for (std::size_t y = 1; y < config.height; ++y)
		for (std::size_t x = 0; x < config.width; ++x)
			inspectFace(Index(x, y - 1), Index(x, y));
	if (config.boundary == OmniAtmosphereBoundary::Periodic)
	{
		for (std::size_t y = 0; y < config.height; ++y)
			inspectFace(Index(config.width - 1, y), Index(0, y));
		for (std::size_t x = 0; x < config.width; ++x)
			inspectFace(Index(x, config.height - 1), Index(x, 0));
	}
	// Open molecular/conductive faces intentionally use a zero-normal-gradient
	// outflow condition. The conservative Riemann faces in AdvanceOnce remain
	// responsible for all exchange with the configured ambient reservoir.
	if (invalidState)
		return 0.0;

	double diffusivityBound = 0.0;
	if (config.speciesDiffusion && speciesGradient)
	{
		// Mixture-averaged counter-diffusion couples every species through the
		// zero-net-mass correction. The sum is a conservative row-norm bound for
		// the explicit operator, rather than assuming max(D) is sufficient.
		for (const auto &species : config.species)
			diffusivityBound += species.diffusionCoefficientM2S;
	}
	if (config.thermalConduction && thermalGradient)
	{
		double maximumConductivity = 0.0;
		double minimumHeatCapacityDensity = std::numeric_limits<double>::infinity();
		for (std::size_t cell = 0; cell < state.size(); ++cell)
		{
			if (blocked[cell])
				continue;
			double conductivity = 0.0;
			double heatCapacityDensity = condensedWaterDensity[cell] *
				OmniThermal::LiquidSpecificHeatJKgK;
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double density = speciesState[SpeciesIndex(cell, species)];
				conductivity += density / state[cell].density *
					config.species[species].thermalConductivityWMK;
				heatCapacityDensity += density * SpeciesCv(config.species[species]);
			}
			if (!Finite(conductivity) || conductivity < 0.0 ||
				!Finite(heatCapacityDensity) || !(heatCapacityDensity > 0.0))
			{
				return 0.0;
			}
			maximumConductivity = std::max(maximumConductivity, conductivity);
			minimumHeatCapacityDensity = std::min(
				minimumHeatCapacityDensity, heatCapacityDensity);
		}
		if (maximumConductivity > 0.0 && Finite(minimumHeatCapacityDensity) &&
			minimumHeatCapacityDensity > 0.0)
		{
			diffusivityBound += maximumConductivity / minimumHeatCapacityDensity;
		}
	}
	if (!(diffusivityBound > 0.0) || !Finite(diffusivityBound))
		return std::numeric_limits<double>::infinity();
	return Square(config.scale.cellLengthM) /
		(2.0 * static_cast<double>(dimensions) * diffusivityBound);
}

void OmniAtmosphere::DiffuseSpeciesAndHeat(double dt)
{
	if (!transportActive || (!config.speciesDiffusion && !config.thermalConduction) || !(dt > 0.0))
		return;
	bool speciesGradient = false;
	bool thermalGradient = false;
	auto inspectFace = [&](std::size_t left, std::size_t right) {
		if (left == right || blocked[left] || blocked[right])
			return;
		const auto leftPrimitive = Derive(left, state[left]);
		const auto rightPrimitive = Derive(right, state[right]);
		if (!leftPrimitive.finite || !rightPrimitive.finite)
		{
			speciesGradient = thermalGradient = true;
			return;
		}
		thermalGradient = thermalGradient ||
			std::abs(leftPrimitive.temperature - rightPrimitive.temperature) > 1.0e-9;
		if (!speciesGradient)
		{
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double leftFraction = speciesState[SpeciesIndex(left, species)] / state[left].density;
				const double rightFraction = speciesState[SpeciesIndex(right, species)] / state[right].density;
				if (std::abs(leftFraction - rightFraction) > 1.0e-12)
				{
					speciesGradient = true;
					break;
				}
			}
		}
	};
	for (std::size_t y = 0; y < config.height && !(speciesGradient && thermalGradient); ++y)
		for (std::size_t x = 1; x < config.width && !(speciesGradient && thermalGradient); ++x)
			inspectFace(Index(x - 1, y), Index(x, y));
	for (std::size_t y = 1; y < config.height && !(speciesGradient && thermalGradient); ++y)
		for (std::size_t x = 0; x < config.width && !(speciesGradient && thermalGradient); ++x)
			inspectFace(Index(x, y - 1), Index(x, y));
	if (config.boundary == OmniAtmosphereBoundary::Periodic &&
		!(speciesGradient && thermalGradient))
	{
		for (std::size_t y = 0; y < config.height && !(speciesGradient && thermalGradient); ++y)
			inspectFace(Index(config.width - 1, y), Index(0, y));
		for (std::size_t x = 0; x < config.width && !(speciesGradient && thermalGradient); ++x)
			inspectFace(Index(x, config.height - 1), Index(x, 0));
	}
	if (!speciesGradient && !thermalGradient)
	{
		transportActive = false;
		return;
	}
	const double factor = dt / config.scale.cellLengthM;
	const double energyBefore = TotalEnergyJ();
	next = state;
	speciesNext = speciesState;
	auto conductivity = [&](std::size_t cell) {
		double result = 0.0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
			result += SpeciesMassFraction(cell % config.width, cell / config.width, species) *
				config.species[species].thermalConductivityWMK;
		return result;
	};
	bool gpuThermalApplied = false;
	if (config.thermalConduction && thermalGradient && OmniCompute::IsGPUAvailable())
	{
		std::vector<float> temperatures(state.size(), 0.0f);
		std::vector<float> conductivities(state.size(), 0.0f);
		for (std::size_t cell = 0; cell < state.size(); ++cell)
		{
			temperatures[cell] = static_cast<float>(Derive(cell, state[cell]).temperature);
			conductivities[cell] = static_cast<float>(conductivity(cell));
		}
		const OmniThermalDiffusionInput input{
			.width = config.width,
			.height = config.height,
			.periodic = config.boundary == OmniAtmosphereBoundary::Periodic,
			.timestepOverCellLengthSquared = static_cast<float>(
				dt / (config.scale.cellLengthM * config.scale.cellLengthM)),
			.temperature = temperatures,
			.conductivity = conductivities,
			.blocked = blocked,
		};
		std::vector<float> energyDelta;
		std::string error;
		if (OmniCompute::RunThermalDiffusion(input, energyDelta, error))
		{
			for (std::size_t cell = 0; cell < state.size(); ++cell)
				next[cell].totalEnergy += energyDelta[cell];
			gpuThermalApplied = true;
		}
		else
		{
			OmniCompute::ResetBackend("runtime thermal diffusion fallback: " + error);
		}
	}
	auto applyFace = [&](std::size_t left, std::size_t right) {
		if (left == right || blocked[left] || blocked[right])
			return;
		const auto leftPrimitive = Derive(left, state[left]);
		const auto rightPrimitive = Derive(right, state[right]);
		if (!leftPrimitive.finite || !rightPrimitive.finite)
			return;
		if (config.speciesDiffusion && speciesGradient)
		{
			std::vector<double> raw(config.species.size(), 0.0);
			std::vector<double> delta(config.species.size(), 0.0);
			double sumRaw = 0.0;
			const double densityFace = 0.5 * (state[left].density + state[right].density);
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double leftFraction = speciesState[SpeciesIndex(left, species)] / state[left].density;
				const double rightFraction = speciesState[SpeciesIndex(right, species)] / state[right].density;
				raw[species] = -densityFace * config.species[species].diffusionCoefficientM2S *
					(rightFraction - leftFraction) / config.scale.cellLengthM;
				sumRaw += raw[species];
			}
			double limiter = 1.0;
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double faceFraction = 0.5 * (
					speciesState[SpeciesIndex(left, species)] / state[left].density +
					speciesState[SpeciesIndex(right, species)] / state[right].density);
				const double flux = raw[species] - faceFraction * sumRaw;
				delta[species] = factor * flux;
				if (delta[species] > 0.0)
				{
					limiter = std::min(limiter,
						speciesNext[SpeciesIndex(left, species)] / delta[species]);
				}
				else if (delta[species] < 0.0)
				{
					limiter = std::min(limiter,
						speciesNext[SpeciesIndex(right, species)] / -delta[species]);
				}
			}
			limiter = std::clamp(limiter, 0.0, 1.0);
			std::vector<double> limitedTransfer(config.species.size(), 0.0);
			std::vector<double> energyTransfer(config.species.size(), 0.0);
			bool transferPresent = false;
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double limited = limiter * delta[species];
				limitedTransfer[species] = limited;
				if (limited != 0.0)
				{
					transferPresent = true;
					// Mixture-averaged counter-diffusion carries species
					// enthalpy even though its summed mass flux is zero.  In
					// particular, H2O must carry its latent energy; moving only
					// the density lets a cold cell accumulate condensate without
					// the energy required by the conservative state.
					const double donorTemperature = limited > 0.0
						? leftPrimitive.temperature : rightPrimitive.temperature;
					const double specificEnthalpy =
						config.species[species].specificHeatCpJKgK * donorTemperature +
						(species == OMNI_SPECIES_H2O
							? OmniThermal::LatentHeatVaporizationJPerKg : 0.0);
					energyTransfer[species] = limited * specificEnthalpy;
					if (!Finite(specificEnthalpy) || !Finite(energyTransfer[species]))
						throw std::runtime_error("non-finite species enthalpy transfer");
				}
			}
			double thermodynamicLimiter = 1.0;
			if (transferPresent)
			{
				std::vector<double> candidateLeft(config.species.size(), 0.0);
				std::vector<double> candidateRight(config.species.size(), 0.0);
				auto thermodynamicallyValid = [&](double scale) {
					double leftEnergy = next[left].totalEnergy;
					double rightEnergy = next[right].totalEnergy;
					for (std::size_t species = 0; species < config.species.size(); ++species)
					{
						const double massTransfer = scale * limitedTransfer[species];
						candidateLeft[species] =
							speciesNext[SpeciesIndex(left, species)] - massTransfer;
						candidateRight[species] =
							speciesNext[SpeciesIndex(right, species)] + massTransfer;
						leftEnergy -= scale * energyTransfer[species];
						rightEnergy += scale * energyTransfer[species];
					}
					return OmniValidateSerializedAtmosphereCell(
							config, candidateLeft,
							next[left].momentumX, next[left].momentumY,
							leftEnergy, condensedWaterDensity[left], true) &&
						OmniValidateSerializedAtmosphereCell(
							config, candidateRight,
							next[right].momentumX, next[right].momentumY,
							rightEnergy, condensedWaterDensity[right], true);
				};
				if (!thermodynamicallyValid(1.0))
				{
					thermodynamicLimiter = 0.0;
					if (thermodynamicallyValid(0.0))
					{
						double lower = 0.0;
						double upper = 1.0;
						for (int iteration = 0; iteration < 64; ++iteration)
						{
							const double middle = 0.5 * (lower + upper);
							if (thermodynamicallyValid(middle))
								lower = middle;
							else
								upper = middle;
						}
						thermodynamicLimiter = lower;
					}
				}
			}
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double limited = thermodynamicLimiter * limitedTransfer[species];
				speciesNext[SpeciesIndex(left, species)] -= limited;
				speciesNext[SpeciesIndex(right, species)] += limited;
				if (limited != 0.0)
				{
					next[left].totalEnergy -= thermodynamicLimiter * energyTransfer[species];
					next[right].totalEnergy += thermodynamicLimiter * energyTransfer[species];
				}
			}
		}
		if (config.thermalConduction && thermalGradient && !gpuThermalApplied)
		{
			const double k = 0.5 * (conductivity(left) + conductivity(right));
			const double heatFlux = -k * (rightPrimitive.temperature - leftPrimitive.temperature) /
				config.scale.cellLengthM;
			next[left].totalEnergy -= factor * heatFlux;
			next[right].totalEnergy += factor * heatFlux;
		}
	};

	for (std::size_t y = 0; y < config.height; ++y)
		for (std::size_t x = 1; x < config.width; ++x)
			applyFace(Index(x - 1, y), Index(x, y));
	for (std::size_t y = 1; y < config.height; ++y)
		for (std::size_t x = 0; x < config.width; ++x)
			applyFace(Index(x, y - 1), Index(x, y));
	if (config.boundary == OmniAtmosphereBoundary::Periodic)
	{
		for (std::size_t y = 0; y < config.height; ++y)
			applyFace(Index(config.width - 1, y), Index(0, y));
		for (std::size_t x = 0; x < config.width; ++x)
			applyFace(Index(x, config.height - 1), Index(x, 0));
	}

	for (std::size_t cell = 0; cell < state.size(); ++cell)
	{
		if (blocked[cell])
			continue;
		double sum = 0.0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
		{
			auto &density = speciesNext[SpeciesIndex(cell, species)];
			if (!Finite(density) || density < 0.0)
			{
				const double old = density;
				density = 0.0;
				if (Finite(old))
					ledger.numericalSpeciesCorrectionKg[species] += (density - old) * config.scale.cellVolumeM3();
			}
			sum += density;
		}
		next[cell].density = sum;
		ApplyFloors(next[cell]);
		if (next[cell].density != sum)
			RescaleSpeciesToDensity(speciesNext, cell, next[cell].density);
	}
	state.swap(next);
	speciesState.swap(speciesNext);
	ledger.thermalConductionEnergyResidualJ += TotalEnergyJ() - energyBefore;
}

void OmniAtmosphere::ApplyGravity(double dt)
{
	if (config.gravityY == 0.0 || !(dt > 0.0))
		return;
	const double volume = config.scale.cellVolumeM3();
	for (std::size_t cell = 0; cell < state.size(); ++cell)
	{
		if (blocked[cell])
			continue;
		auto &value = state[cell];
		const double oldMomentum = value.momentumY;
		const double oldKinetic = 0.5 * (Square(value.momentumX) + Square(value.momentumY)) / value.density;
		value.momentumY += value.density * config.gravityY * dt;
		const double newKinetic = 0.5 * (Square(value.momentumX) + Square(value.momentumY)) / value.density;
		value.totalEnergy += newKinetic - oldKinetic;
		ledger.sourceMomentumY += (value.momentumY - oldMomentum) * volume;
		ledger.sourceEnergyJ += (newKinetic - oldKinetic) * volume;
	}
}

void OmniAtmosphere::EquilibrateWaterPhase()
{
	if (!phaseActive || !config.waterPhaseEquilibrium || config.species.size() <= OMNI_SPECIES_H2O)
		return;
	const double waterGasConstant = SpeciesGasConstant(config.species[OMNI_SPECIES_H2O]);
	const double waterCv = SpeciesCv(config.species[OMNI_SPECIES_H2O]);
	const double volume = config.scale.cellVolumeM3();
	for (std::size_t cell = 0; cell < state.size(); ++cell)
	{
		if (blocked[cell])
			continue;
		auto &vapour = speciesState[SpeciesIndex(cell, OMNI_SPECIES_H2O)];
		auto &condensed = condensedWaterDensity[cell];
		const double totalWater = vapour + condensed;
		if (!(totalWater > 0.0) || !(state[cell].density > 0.0))
			continue;
		const double oldDensity = state[cell].density;
		const double oldMomentumX = state[cell].momentumX;
		const double oldMomentumY = state[cell].momentumY;
		const double velocitySquared =
			(Square(oldMomentumX) + Square(oldMomentumY)) / Square(oldDensity);
		double dryHeatCapacityDensity = 0.0;
		double dryGasDensity = 0.0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
		{
			if (species != OMNI_SPECIES_H2O)
			{
				dryGasDensity += speciesState[SpeciesIndex(cell, species)];
				dryHeatCapacityDensity += speciesState[SpeciesIndex(cell, species)] * SpeciesCv(config.species[species]);
			}
		}
		auto equilibriumEnergy = [&](double temperature) {
			const double saturationDensity = OmniThermal::SaturationPressurePa(temperature) /
				(waterGasConstant * temperature);
			const double vapourAtTemperature = std::clamp(saturationDensity, 0.0, totalWater);
			const double condensedAtTemperature = totalWater - vapourAtTemperature;
			const double internal = (dryHeatCapacityDensity + vapourAtTemperature * waterCv +
				condensedAtTemperature * OmniThermal::LiquidSpecificHeatJKgK) * temperature +
				vapourAtTemperature * OmniThermal::LatentHeatVaporizationJPerKg;
			// Gas transferred from the condensed phase inherits the cell velocity.
			// Include its kinetic energy in the root so the final momentum rescale
			// does not invalidate the solved thermodynamic state.
			return internal + 0.5 * (dryGasDensity + vapourAtTemperature) * velocitySquared;
		};
		const double totalEnergyDensity = state[cell].totalEnergy;
		double lower = 1.0;
		double upper = 2000.0;
		while (equilibriumEnergy(upper) < totalEnergyDensity && upper < 100000.0)
			upper *= 2.0;
		for (int iteration = 0; iteration < 96; ++iteration)
		{
			const double middle = 0.5 * (lower + upper);
			if (equilibriumEnergy(middle) < totalEnergyDensity)
				lower = middle;
			else
				upper = middle;
		}
		const double equilibriumTemperature = 0.5 * (lower + upper);
		const double targetVapour = std::clamp(
			OmniThermal::SaturationPressurePa(equilibriumTemperature) /
				(waterGasConstant * equilibriumTemperature),
			0.0,
			totalWater);
		const double transferToVapour = targetVapour - vapour;
		if (std::abs(transferToVapour) > 1.0e-14)
		{
			vapour = targetVapour;
			condensed = totalWater - targetVapour;
			state[cell].density += transferToVapour;
			const double velocityScale = state[cell].density / oldDensity;
			state[cell].momentumX *= velocityScale;
			state[cell].momentumY *= velocityScale;
			const double mass = transferToVapour * volume;
			ledger.phaseTransferWaterMassKg += mass;
			ledger.phaseTransferLatentEnergyJ += mass * OmniThermal::LatentHeatVaporizationJPerKg;
			ledger.sourceSpeciesMassKg[OMNI_SPECIES_H2O] += mass;
			ledger.sourceMomentumX += (state[cell].momentumX - oldMomentumX) * volume;
			ledger.sourceMomentumY += (state[cell].momentumY - oldMomentumY) * volume;
			// state density was advanced by an independent floating-point add, so
			// a cell sitting on the density floor can finish the substep with its
			// species channels totalling one ULP less. Phase equilibrium is the
			// last pass to touch species state, so nothing downstream repairs the
			// gap and the cell stays permanently unserializable. Close it on the
			// largest dry channel: the solved vapour/condensed partition stays
			// bit-exact, so the equilibrium this pass just established holds.
			RepairSpeciesFloorOnDryChannel(cell, dryGasDensity + targetVapour);
		}

	}
	phaseActive = TotalCondensedWaterMassKg() > 0.0 || TotalSpeciesMassKg(OMNI_SPECIES_H2O) > 0.0;
}

bool OmniAtmosphere::ApplySerializableEnergyFloor(
	std::size_t cell,
	bool recordLedgerCorrection)
{
	if (cell >= state.size())
		return false;
	const double volume = config.scale.cellVolumeM3();
	const auto speciesBegin = speciesState.data() + SpeciesIndex(cell, 0);
	double requiredTotalEnergy = 0.0;
	if (!ComputeSerializedAtmosphereRequiredEnergy(
			config,
			std::span<const double>(speciesBegin, config.species.size()),
			state[cell].momentumX, state[cell].momentumY,
			condensedWaterDensity[cell], requiredTotalEnergy) ||
		!Finite(state[cell].totalEnergy))
	{
		return false;
	}
	double density = 0.0;
	double heatCapacityDensity = condensedWaterDensity[cell] *
		OmniThermal::LiquidSpecificHeatJKgK;
	double gasConstantDensity = 0.0;
	double waterDensity = 0.0;
	for (std::size_t species = 0; species < config.species.size(); ++species)
	{
		const double value = speciesBegin[species];
		density += value;
		heatCapacityDensity += value * SpeciesCv(config.species[species]);
		gasConstantDensity += value * SpeciesGasConstant(config.species[species]);
		if (species == OMNI_SPECIES_H2O)
			waterDensity = value;
	}
	if (!(density > 0.0) || !(gasConstantDensity > 0.0))
		return false;
	const double kinetic = 0.5 * (
		Square(state[cell].momentumX) + Square(state[cell].momentumY)) / density;
	const double latent = waterDensity * OmniThermal::LatentHeatVaporizationJPerKg;
	const double minimumTemperature = std::max(
		1.0, config.pressureFloor / gasConstantDensity);
	const double oldTemperature =
		(state[cell].totalEnergy - kinetic - latent) / heatCapacityDensity;
	const bool pressureFloorRequired =
		Finite(kinetic) && Finite(latent) && Finite(heatCapacityDensity) &&
		Finite(gasConstantDensity) && Finite(minimumTemperature) &&
		Finite(oldTemperature) && oldTemperature * gasConstantDensity < config.pressureFloor;
	auto serializedStateValid = [&] {
		return OmniValidateSerializedAtmosphereCell(
			config,
			std::span<const double>(speciesBegin, config.species.size()),
			state[cell].momentumX, state[cell].momentumY,
			state[cell].totalEnergy, condensedWaterDensity[cell], true);
	};
	if (serializedStateValid())
		return true;
	const double oldEnergy = state[cell].totalEnergy;
	// Solver floors are ordinary recorded numerical corrections and may
	// canonicalize any finite deficit. Save/heartbeat boundary repair is much
	// narrower: it may bridge only a handful of representable round-off steps,
	// never hide a materially invalid coupled state as a synthetic source.
	if (recordLedgerCorrection && state[cell].totalEnergy < requiredTotalEnergy)
		state[cell].totalEnergy = requiredTotalEnergy;
	for (int attempt = 0; attempt < 8 && !serializedStateValid(); ++attempt)
	{
		state[cell].totalEnergy = std::nextafter(
			state[cell].totalEnergy, std::numeric_limits<double>::infinity());
	}
	if (!serializedStateValid())
	{
		// A floor helper must never turn a failed canonicalization attempt
		// into an unaccounted state mutation. Leave the original invalid
		// value intact so the runtime/save validators fail closed.
		state[cell].totalEnergy = oldEnergy;
		return false;
	}
	if (Finite(oldEnergy) && Finite(state[cell].totalEnergy) &&
		state[cell].totalEnergy > oldEnergy)
	{
		const double correctionJ =
			(state[cell].totalEnergy - oldEnergy) * volume;
		if (recordLedgerCorrection)
		{
			ledger.numericalEnergyCorrectionJ += correctionJ;
			ledger.energyFloorHits++;
			if (pressureFloorRequired)
				ledger.pressureFloorHits++;
		}
		else
			pendingSourceEnergyJ += correctionJ;
	}
	return true;
}

void OmniAtmosphere::ApplySerializableEnergyFloors(bool recordLedgerCorrection)
{
	// Apply the exact OPS v3 mixture/latent-energy floor to every authoritative
	// cell, including dry and blocked placeholders. Species diffusion and GPU
	// heat transport can lower a dry cell below the legacy gamma-only bound even
	// when phase equilibrium has no water work to perform.
	for (std::size_t cell = 0; cell < state.size(); ++cell)
		ApplySerializableEnergyFloor(cell, recordLedgerCorrection);
}

bool OmniAtmosphere::EnsureSerializableRegion(
	std::size_t x,
	std::size_t y,
	std::size_t width,
	std::size_t height)
{
	// A save may canonicalize a finite energy-floor roundoff, but it must never
	// wash a corrupt live conservative density into the species-derived payload.
	// Validate the live density/species coupling before applying any repair so a
	// failed boundary remains side-effect free and fail-closed.
	if (x > config.width || y > config.height ||
		width > config.width - x || height > config.height - y)
	{
		return false;
	}
	std::vector<std::size_t> cells;
	cells.reserve(width * height);
	for (std::size_t row = y; row < y + height; ++row)
	{
		for (std::size_t column = x; column < x + width; ++column)
		{
			const auto cell = Index(column, row);
			cells.push_back(cell);
			if (!Finite(state[cell].density) || state[cell].density < config.densityFloor ||
				!Finite(state[cell].momentumX) || !Finite(state[cell].momentumY) ||
				!Finite(state[cell].totalEnergy) || !Finite(condensedWaterDensity[cell]) ||
				condensedWaterDensity[cell] < 0.0)
			{
				return false;
			}
			double speciesDensity = 0.0;
			for (std::size_t species = 0; species < config.species.size(); ++species)
			{
				const double value = speciesState[SpeciesIndex(cell, species)];
				if (!Finite(value) || value < 0.0)
					return false;
				speciesDensity += value;
			}
			const double densityScale = std::max({
				std::abs(state[cell].density), std::abs(speciesDensity), 1.0 });
			const double densityTolerance = std::max(
				256.0 * std::numeric_limits<double>::epsilon() * densityScale,
				1.0e-12 * densityScale);
			if (!Finite(speciesDensity) ||
				std::abs(state[cell].density - speciesDensity) > densityTolerance)
			{
				return false;
			}
		}
	}
	std::vector<double> oldEnergies;
	oldEnergies.reserve(cells.size());
	for (const auto cell : cells)
		oldEnergies.push_back(state[cell].totalEnergy);
	const double oldPendingSourceEnergyJ = pendingSourceEnergyJ;
	auto rollback = [&] {
		for (std::size_t index = 0; index < cells.size(); ++index)
			state[cells[index]].totalEnergy = oldEnergies[index];
		pendingSourceEnergyJ = oldPendingSourceEnergyJ;
	};
	for (const auto cell : cells)
	{
		if (!ApplySerializableEnergyFloor(cell, false))
		{
			rollback();
			return false;
		}
	}
	return true;
}

bool OmniAtmosphere::EnsureSerializableState()
{
	return EnsureSerializableRegion(0, 0, config.width, config.height);
}

void OmniAtmosphere::BeginLedger()
{
	ledger = {};
	ledger.initialSpeciesMassKg.resize(config.species.size(), 0.0);
	ledger.finalSpeciesMassKg.resize(config.species.size(), 0.0);
	ledger.sourceSpeciesMassKg = pendingSourceSpeciesMassKg;
	ledger.boundarySpeciesInKg.resize(config.species.size(), 0.0);
	ledger.boundarySpeciesOutKg.resize(config.species.size(), 0.0);
	ledger.numericalSpeciesCorrectionKg.resize(config.species.size(), 0.0);
	for (std::size_t species = 0; species < config.species.size(); ++species)
		ledger.initialSpeciesMassKg[species] = TotalSpeciesMassKg(species) - pendingSourceSpeciesMassKg[species];
	ledger.initialCondensedWaterMassKg = TotalCondensedWaterMassKg();
	ledger.sourceMassKg = pendingSourceMassKg;
	ledger.sourceMomentumX = pendingSourceMomentumX;
	ledger.sourceMomentumY = pendingSourceMomentumY;
	ledger.sourceEnergyJ = pendingSourceEnergyJ;
	ledger.initialMassKg = TotalMassKg() - pendingSourceMassKg;
	ledger.initialMomentumX = TotalMomentumX();
	ledger.initialMomentumX -= pendingSourceMomentumX;
	ledger.initialMomentumY = TotalMomentumY() - pendingSourceMomentumY;
	ledger.initialEnergyJ = TotalEnergyJ() - pendingSourceEnergyJ;
}

void OmniAtmosphere::FinishLedger()
{
	ledger.finalMassKg = TotalMassKg();
	ledger.finalMomentumX = TotalMomentumX();
	ledger.finalMomentumY = TotalMomentumY();
	ledger.finalEnergyJ = TotalEnergyJ();
	for (std::size_t species = 0; species < config.species.size(); ++species)
		ledger.finalSpeciesMassKg[species] = TotalSpeciesMassKg(species);
	ledger.finalCondensedWaterMassKg = TotalCondensedWaterMassKg();
	pendingSourceMassKg = 0.0;
	pendingSourceMomentumX = 0.0;
	pendingSourceMomentumY = 0.0;
	pendingSourceEnergyJ = 0.0;
	std::fill(pendingSourceSpeciesMassKg.begin(), pendingSourceSpeciesMassKg.end(), 0.0);
}

void OmniAtmosphere::Step()
{
	BeginLedger();
	bool openBoundaryEvent = false;
	if (config.boundary == OmniAtmosphereBoundary::Open)
	{
		const auto ambient = AmbientState();
		auto differsFromAmbient = [&ambient](const OmniAtmosphereConservative &cell) {
			const double densityScale = std::max(std::abs(ambient.density), 1.0);
			const double energyScale = std::max(std::abs(ambient.totalEnergy), 1.0);
			return std::abs(cell.density - ambient.density) > densityScale * 1.0e-12 ||
				std::abs(cell.momentumX) > densityScale * 1.0e-12 ||
				std::abs(cell.momentumY) > densityScale * 1.0e-12 ||
				std::abs(cell.totalEnergy - ambient.totalEnergy) > energyScale * 1.0e-12;
		};
		for (std::size_t x = 0; x < config.width && !openBoundaryEvent; ++x)
			openBoundaryEvent = differsFromAmbient(state[Index(x, 0)]) || differsFromAmbient(state[Index(x, config.height - 1)]);
		for (std::size_t y = 0; y < config.height && !openBoundaryEvent; ++y)
			openBoundaryEvent = differsFromAmbient(state[Index(0, y)]) || differsFromAmbient(state[Index(config.width - 1, y)]);
	}
	const bool acoustic = config.execution == OmniAtmosphereExecution::ReferenceCompressible || pendingEvent || compressibleActive || openBoundaryEvent;
	ledger.acousticRoute = acoustic;
	double maximumSignal = 0.0;
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		if (blocked[index])
			continue;
		const auto primitive = Derive(index, state[index]);
		if (!primitive.finite)
			continue;
		const double acousticSignal = acoustic ? primitive.soundSpeed : 0.0;
		const double multidimensionalSignal =
			std::abs(primitive.velocityX) + acousticSignal +
			std::abs(primitive.velocityY) + acousticSignal;
		maximumSignal = std::max(maximumSignal, multidimensionalSignal);
	}
	const std::size_t maximum = config.execution == OmniAtmosphereExecution::ReferenceCompressible
		? config.maximumReferenceSubsteps
		: config.maximumRuntimeSubsteps;
	const double acousticStableTimestep = maximumSignal > 0.0
		? config.cfl * config.scale.cellLengthM / maximumSignal
		: std::numeric_limits<double>::infinity();
	const bool transportMayBeCreated = pendingEvent || compressibleActive ||
		openBoundaryEvent ||
		(config.execution == OmniAtmosphereExecution::ReferenceCompressible &&
			CompressibleFeaturesPresent());
	const double stableSubstep = std::min({
		config.scale.timestepS, acousticStableTimestep,
		TransportStableTimestep(transportMayBeCreated) });
	std::size_t requiredSubsteps = 1;
	bool stabilityExceeded = false;
	if (!(stableSubstep > 0.0) || !Finite(stableSubstep))
	{
		requiredSubsteps = maximum;
		stabilityExceeded = true;
	}
	else
	{
		const double requestedSubsteps = config.scale.timestepS / stableSubstep;
		if (!Finite(requestedSubsteps) || requestedSubsteps > static_cast<double>(maximum))
		{
			requiredSubsteps = maximum;
			stabilityExceeded = true;
		}
		else
		{
			requiredSubsteps = std::max<std::size_t>(
				1, static_cast<std::size_t>(std::ceil(requestedSubsteps)));
		}
	}
	const std::size_t substeps = requiredSubsteps;
	ledger.timestepLimited = stabilityExceeded;
	ledger.substeps = substeps;
	ledger.requestedTimestepS = config.scale.timestepS;
	const double substep = ledger.timestepLimited
		? stableSubstep
		: config.scale.timestepS / static_cast<double>(substeps);
	ledger.advancedTimestepS = substep * static_cast<double>(substeps);
	for (std::size_t index = 0; index < substeps; ++index)
	{
		if (acoustic || maximumSignal > 0.0)
		{
			AdvanceOnce(substep, acoustic);
			transportActive = true;
		}
		ReportFirstFloorInvariantViolation("AdvanceOnce");
		DiffuseSpeciesAndHeat(substep);
		ReportFirstFloorInvariantViolation("DiffuseSpeciesAndHeat");
		ApplyGravity(substep);
		ReportFirstFloorInvariantViolation("ApplyGravity");
		ApplySerializableEnergyFloors();
		ReportFirstFloorInvariantViolation("ApplySerializableEnergyFloors");
		EquilibrateWaterPhase();
		ReportFirstFloorInvariantViolation("EquilibrateWaterPhase");
	}
	pendingEvent = false;
	compressibleActive = config.execution == OmniAtmosphereExecution::RuntimeLowMach &&
		(acoustic || maximumSignal > 0.0) && CompressibleFeaturesPresent();
	FinishLedger();
}

void OmniAtmosphere::StepReference(double timestepS)
{
	if (!Finite(timestepS) || timestepS <= 0.0)
		throw std::invalid_argument("invalid OmniAtmosphere reference timestep");
	BeginLedger();
	double maximumSignal = 0.0;
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		if (blocked[index])
			continue;
		const auto primitive = Derive(index, state[index]);
		if (primitive.finite)
		{
			maximumSignal = std::max(maximumSignal,
				std::abs(primitive.velocityX) + primitive.soundSpeed +
				std::abs(primitive.velocityY) + primitive.soundSpeed);
		}
	}
	const std::size_t maximum = config.maximumReferenceSubsteps;
	const double acousticStableTimestep = maximumSignal > 0.0
		? config.cfl * config.scale.cellLengthM / maximumSignal
		: std::numeric_limits<double>::infinity();
	const bool transportMayBeCreated = CompressibleFeaturesPresent();
	const double stableSubstep = std::min({
		timestepS, acousticStableTimestep,
		TransportStableTimestep(transportMayBeCreated) });
	std::size_t requiredSubsteps = 1;
	bool stabilityExceeded = false;
	if (!(stableSubstep > 0.0) || !Finite(stableSubstep))
	{
		requiredSubsteps = maximum;
		stabilityExceeded = true;
	}
	else
	{
		const double requestedSubsteps = timestepS / stableSubstep;
		if (!Finite(requestedSubsteps) || requestedSubsteps > static_cast<double>(maximum))
		{
			requiredSubsteps = maximum;
			stabilityExceeded = true;
		}
		else
		{
			requiredSubsteps = std::max<std::size_t>(
				1, static_cast<std::size_t>(std::ceil(requestedSubsteps)));
		}
	}
	const std::size_t substeps = requiredSubsteps;
	ledger.timestepLimited = stabilityExceeded;
	ledger.substeps = substeps;
	ledger.requestedTimestepS = timestepS;
	const double substep = ledger.timestepLimited
		? stableSubstep
		: timestepS / static_cast<double>(substeps);
	ledger.advancedTimestepS = substep * static_cast<double>(substeps);
	for (std::size_t index = 0; index < substeps; ++index)
	{
		AdvanceOnce(substep, true);
		ReportFirstFloorInvariantViolation("AdvanceOnce/acoustic");
		DiffuseSpeciesAndHeat(substep);
		ReportFirstFloorInvariantViolation("DiffuseSpeciesAndHeat/acoustic");
		ApplyGravity(substep);
		ReportFirstFloorInvariantViolation("ApplyGravity/acoustic");
		ApplySerializableEnergyFloors();
		ReportFirstFloorInvariantViolation("ApplySerializableEnergyFloors/acoustic");
		EquilibrateWaterPhase();
		ReportFirstFloorInvariantViolation("EquilibrateWaterPhase/acoustic");
	}
	pendingEvent = false;
	compressibleActive = CompressibleFeaturesPresent();
	ledger.acousticRoute = true;
	FinishLedger();
}

const OmniAtmosphereConservative &OmniAtmosphere::State(std::size_t x, std::size_t y) const
{
	if (x >= config.width || y >= config.height)
		throw std::out_of_range("OmniAtmosphere state coordinate");
	return state[Index(x, y)];
}

OmniAtmospherePrimitive OmniAtmosphere::Primitive(std::size_t x, std::size_t y) const
{
	return Derive(Index(x, y), State(x, y));
}

double OmniAtmosphere::SpeciesMassDensity(std::size_t x, std::size_t y, std::size_t species) const
{
	if (x >= config.width || y >= config.height || species >= config.species.size())
		throw std::out_of_range("OmniAtmosphere species coordinate");
	return speciesState[SpeciesIndex(Index(x, y), species)];
}

double OmniAtmosphere::SpeciesMassFraction(std::size_t x, std::size_t y, std::size_t species) const
{
	const auto &cell = State(x, y);
	return cell.density > 0.0 ? SpeciesMassDensity(x, y, species) / cell.density : 0.0;
}

double OmniAtmosphere::SpeciesPartialPressurePa(std::size_t x, std::size_t y, std::size_t species) const
{
	const auto primitive = Primitive(x, y);
	return SpeciesMassDensity(x, y, species) * SpeciesGasConstant(config.species.at(species)) * primitive.temperature;
}

double OmniAtmosphere::TotalSpeciesMassKg(std::size_t species) const
{
	if (species >= config.species.size())
		throw std::out_of_range("OmniAtmosphere species index");
	double total = 0.0;
	for (std::size_t cell = 0; cell < state.size(); ++cell)
		total += speciesState[SpeciesIndex(cell, species)];
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalCondensedWaterMassKg() const
{
	double total = 0.0;
	for (double density : condensedWaterDensity)
		total += density;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalMassKg() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.density;
	return total * config.scale.cellVolumeM3() + TotalCondensedWaterMassKg();
}

double OmniAtmosphere::TotalMomentumX() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.momentumX;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalMomentumY() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.momentumY;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::TotalEnergyJ() const
{
	double total = 0.0;
	for (const auto &cell : state)
		total += cell.totalEnergy;
	return total * config.scale.cellVolumeM3();
}

double OmniAtmosphere::MinimumDensity() const
{
	double minimum = std::numeric_limits<double>::infinity();
	for (const auto &cell : state)
	{
		if (!Finite(cell.density))
			return std::numeric_limits<double>::quiet_NaN();
		minimum = std::min(minimum, cell.density);
	}
	return minimum;
}

double OmniAtmosphere::MaximumDensity() const
{
	double maximum = -std::numeric_limits<double>::infinity();
	for (const auto &cell : state)
	{
		if (!Finite(cell.density))
			return std::numeric_limits<double>::quiet_NaN();
		maximum = std::max(maximum, cell.density);
	}
	return maximum;
}

double OmniAtmosphere::MinimumPressure() const
{
	double minimum = std::numeric_limits<double>::infinity();
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		const auto primitive = Derive(index, state[index]);
		if (!primitive.finite)
			return std::numeric_limits<double>::quiet_NaN();
		minimum = std::min(minimum, primitive.pressure);
	}
	return minimum;
}

double OmniAtmosphere::MaximumPressure() const
{
	double maximum = -std::numeric_limits<double>::infinity();
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		const auto primitive = Derive(index, state[index]);
		if (!primitive.finite)
			return std::numeric_limits<double>::quiet_NaN();
		maximum = std::max(maximum, primitive.pressure);
	}
	return maximum;
}

double OmniAtmosphere::MinimumTemperature() const
{
	double minimum = std::numeric_limits<double>::infinity();
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		const auto primitive = Derive(index, state[index]);
		if (!primitive.finite)
			return std::numeric_limits<double>::quiet_NaN();
		minimum = std::min(minimum, primitive.temperature);
	}
	return minimum;
}

double OmniAtmosphere::MaximumTemperature() const
{
	double maximum = -std::numeric_limits<double>::infinity();
	for (std::size_t index = 0; index < state.size(); ++index)
	{
		const auto primitive = Derive(index, state[index]);
		if (!primitive.finite)
			return std::numeric_limits<double>::quiet_NaN();
		maximum = std::max(maximum, primitive.temperature);
	}
	return maximum;
}

uint64_t OmniAtmosphere::NonFiniteStateCells() const
{
	bool reportedInvalidCell = false;
	uint64_t count = 0;
	for (std::size_t cell = 0; cell < state.size(); ++cell)
	{
		const auto speciesBegin = speciesState.data() + SpeciesIndex(cell, 0);
		double speciesDensity = 0.0;
		for (std::size_t species = 0; species < config.species.size(); ++species)
			speciesDensity += speciesBegin[species];
		const double densityScale = std::max({
			std::abs(state[cell].density), std::abs(speciesDensity), 1.0 });
		const double densityTolerance = std::max(
			256.0 * std::numeric_limits<double>::epsilon() * densityScale,
			1.0e-12 * densityScale);
		// Runtime heartbeats must fail before an OPS checkpoint discovers a
		// conservative state that cannot be serialized. Preserve the historical
		// live-state/EOS check as well: serialized v3 reconstructs density from
		// species and therefore cannot by itself detect a corrupt runtime density.
		const bool liveStateValid = Derive(cell, state[cell]).finite &&
			Finite(state[cell].density) &&
			std::abs(state[cell].density - speciesDensity) <= densityTolerance;
		const bool serializedStateValid = OmniValidateSerializedAtmosphereCell(
				config,
				std::span<const double>(speciesBegin, config.species.size()),
				state[cell].momentumX, state[cell].momentumY,
				state[cell].totalEnergy, condensedWaterDensity[cell], true);
		if (!liveStateValid || !serializedStateValid)
		{
			// Keep the first failing cell diagnosable in a long-running client;
			// the count remains the machine-readable gate value.  This is emitted
			// only on an already-invalid state and never relaxes validation.
			double requiredEnergy = 0.0;
			const bool requirementFinite = ComputeSerializedAtmosphereRequiredEnergy(
				config,
				std::span<const double>(speciesBegin, config.species.size()),
				state[cell].momentumX, state[cell].momentumY,
				condensedWaterDensity[cell], requiredEnergy);
			if (!reportedInvalidCell)
			{
				reportedInvalidCell = true;
				std::fprintf(stderr,
				"invalid OmniAtmosphere serializable cell: cell=%zu x=%zu y=%zu "
				"density=%.17g species_density=%.17g momentum_x=%.17g momentum_y=%.17g total_energy=%.17g "
				"condensed_water=%.17g required_energy=%.17g requirement_finite=%d\n",
				cell, cell % config.width, cell / config.width, state[cell].density, speciesDensity,
				state[cell].momentumX, state[cell].momentumY, state[cell].totalEnergy,
				condensedWaterDensity[cell], requiredEnergy, requirementFinite ? 1 : 0);
			}
			++count;
		}
	}
	return count;
}

float OmniAtmosphere::LegacyPressure(std::size_t x, std::size_t y) const
{
	const auto primitive = Primitive(x, y);
	return static_cast<float>((primitive.pressure - config.referencePressure) / config.legacyPressureScalePa);
}

float OmniAtmosphere::LegacyVelocityX(std::size_t x, std::size_t y) const
{
	return static_cast<float>(Primitive(x, y).velocityX);
}

float OmniAtmosphere::LegacyVelocityY(std::size_t x, std::size_t y) const
{
	return static_cast<float>(Primitive(x, y).velocityY);
}

float OmniAtmosphere::LegacyTemperature(std::size_t x, std::size_t y) const
{
	return static_cast<float>(Primitive(x, y).temperature);
}
