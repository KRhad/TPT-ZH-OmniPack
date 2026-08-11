#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Strict-double, backend-independent chemistry contract.  It deliberately
// plans transactions rather than mutating Particle or OmniAtmosphere state;
// the Simulation integration layer remains responsible for committing every
// condensed/gas/energy delta atomically.

enum class OmniReactionPhase
{
	Solid,
	Liquid,
	Gas,
	Aerosol,
};

struct OmniReactionElementCount
{
	std::string symbol;
	int count = 0;
};

struct OmniReactionSpecies
{
	std::string id;
	double molarMassKgPerMol = 0.0;
	int chargeNumber = 0;
	OmniReactionPhase phase = OmniReactionPhase::Gas;
	std::vector<OmniReactionElementCount> elementalComposition;
};

struct OmniReactionTerm
{
	std::size_t species = 0;
	int stoichiometricCoefficient = 0;
};

struct OmniArrheniusRate
{
	// Pseudo-first-order rate for the first heterogeneous runtime reaction.
	// Units are s^-1, dimensionless, J mol^-1 and K respectively.
	double preExponentialPerS = 0.0;
	double temperatureExponent = 0.0;
	double activationEnergyJPerMol = 0.0;
	double referenceTemperatureK = 1.0;
};

struct OmniReactionDefinition
{
	std::string id;
	std::vector<OmniReactionTerm> reactants;
	std::vector<OmniReactionTerm> products;
	OmniArrheniusRate rate;
	// Thermodynamic sign convention: negative is exothermic.
	double enthalpyChangeJPerMol = 0.0;
	bool reversible = false;
};

struct OmniReactionValidation
{
	bool valid = false;
	std::string reason;
	double molarMassResidualKgPerMol = 0.0;
	int chargeResidual = 0;
};

struct OmniCarbonOxidationInput
{
	double carbonMassKg = 0.0;
	double oxygenMassKg = 0.0;
	double temperatureK = 0.0;
	double timestepS = 0.0;
	double activeSurfaceFraction = 1.0;
};

struct OmniCarbonOxidationPlan
{
	bool valid = false;
	double extentMol = 0.0;
	double carbonConsumedKg = 0.0;
	double oxygenConsumedKg = 0.0;
	double carbonDioxideProducedKg = 0.0;
	double heatReleasedJ = 0.0;
	double massResidualKg = 0.0;
	double carbonAtomResidualMol = 0.0;
	double oxygenAtomResidualMol = 0.0;
	double energyResidualJ = 0.0;
	bool limitedByCarbon = false;
	bool limitedByOxygen = false;
};

class OmniReactionRuntime
{
public:
	static constexpr std::size_t CarbonSpecies = 0;
	static constexpr std::size_t OxygenSpecies = 1;
	static constexpr std::size_t CarbonDioxideSpecies = 2;

	OmniReactionRuntime(std::vector<OmniReactionSpecies> species,
		std::vector<OmniReactionDefinition> reactions);

	static OmniReactionRuntime CarbonOxidationV1();
	static std::vector<OmniReactionSpecies> CarbonOxidationSpeciesV1();
	static OmniReactionDefinition CarbonOxidationReactionV1();

	const std::vector<OmniReactionSpecies> &Species() const { return species; }
	const std::vector<OmniReactionDefinition> &Reactions() const { return reactions; }
	OmniReactionValidation ValidateReaction(std::size_t reaction) const;
	OmniCarbonOxidationPlan PlanCarbonOxidation(const OmniCarbonOxidationInput &input) const;

private:
	std::vector<OmniReactionSpecies> species;
	std::vector<OmniReactionDefinition> reactions;
};

static_assert(sizeof(double) == 8, "OmniReactionRuntime requires IEEE-style 64-bit double storage");
