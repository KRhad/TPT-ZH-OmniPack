#include "OmniReactionRuntime.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>

namespace
{
constexpr double UniversalGasConstantJPerMolK = 8.31446261815324;
constexpr double CarbonMolarMassKgPerMol = 0.0120107;
constexpr double OxygenMolarMassKgPerMol = 0.0319988;
constexpr double CarbonDioxideMolarMassKgPerMol = CarbonMolarMassKgPerMol + OxygenMolarMassKgPerMol;
constexpr double CarbonOxidationEnthalpyJPerMol = -393500.0;

bool Finite(double value)
{
	return std::isfinite(value);
}

double SpeciesMoles(const OmniReactionSpecies &species, int coefficient)
{
	return species.molarMassKgPerMol * static_cast<double>(coefficient);
}

void AccumulateAtoms(
	std::map<std::string, int> &atoms,
	const OmniReactionSpecies &species,
	int coefficient,
	int sign)
{
	for (const auto &element : species.elementalComposition)
		atoms[element.symbol] += sign * coefficient * element.count;
}

int AtomCount(const OmniReactionSpecies &species, const char *symbol)
{
	for (const auto &element : species.elementalComposition)
		if (element.symbol == symbol)
			return element.count;
	return 0;
}
}

OmniReactionRuntime::OmniReactionRuntime(
	std::vector<OmniReactionSpecies> newSpecies,
	std::vector<OmniReactionDefinition> newReactions):
	species(std::move(newSpecies)),
	reactions(std::move(newReactions))
{
	if (species.empty() || reactions.empty())
		throw std::invalid_argument("OmniReactionRuntime catalog is empty");
	for (std::size_t index = 0; index < reactions.size(); ++index)
	{
		const auto validation = ValidateReaction(index);
		if (!validation.valid)
			throw std::invalid_argument("invalid OmniReactionRuntime reaction: " + validation.reason);
	}
}

std::vector<OmniReactionSpecies> OmniReactionRuntime::CarbonOxidationSpeciesV1()
{
	return {
		{ "species.carbon", CarbonMolarMassKgPerMol, 0, OmniReactionPhase::Solid, { { "C", 1 } } },
		{ "species.oxygen", OxygenMolarMassKgPerMol, 0, OmniReactionPhase::Gas, { { "O", 2 } } },
		{ "species.carbon-dioxide", CarbonDioxideMolarMassKgPerMol, 0, OmniReactionPhase::Gas,
			{ { "C", 1 }, { "O", 2 } } },
	};
}

OmniReactionDefinition OmniReactionRuntime::CarbonOxidationReactionV1()
{
	return {
		"reaction.carbon-oxidation",
		{ { CarbonSpecies, 1 }, { OxygenSpecies, 1 } },
		{ { CarbonDioxideSpecies, 1 } },
		// The rate parameters are an explicitly game-tuned pseudo-first-order
		// model, not claimed as measured graphite kinetics.  Enthalpy is the
		// standard C(graphite) combustion value used by the data contract.
		{ 2.0e5, 0.0, 80000.0, 1000.0 },
		CarbonOxidationEnthalpyJPerMol,
		false,
	};
}

OmniReactionRuntime OmniReactionRuntime::CarbonOxidationV1()
{
	return OmniReactionRuntime(CarbonOxidationSpeciesV1(), { CarbonOxidationReactionV1() });
}

OmniReactionValidation OmniReactionRuntime::ValidateReaction(std::size_t reactionIndex) const
{
	OmniReactionValidation result;
	if (reactionIndex >= reactions.size())
	{
		result.reason = "reaction index out of range";
		return result;
	}
	const auto &reaction = reactions[reactionIndex];
	if (reaction.id.empty() || reaction.reactants.empty() || reaction.products.empty())
	{
		result.reason = "reaction identity or side is empty";
		return result;
	}
	if (!Finite(reaction.rate.preExponentialPerS) || reaction.rate.preExponentialPerS < 0.0 ||
		!Finite(reaction.rate.temperatureExponent) ||
		!Finite(reaction.rate.activationEnergyJPerMol) || reaction.rate.activationEnergyJPerMol < 0.0 ||
		!Finite(reaction.rate.referenceTemperatureK) || reaction.rate.referenceTemperatureK <= 0.0 ||
		!Finite(reaction.enthalpyChangeJPerMol))
	{
		result.reason = "reaction kinetics or energy is invalid";
		return result;
	}

	std::map<std::string, int> atomResidual;
	double reactantMass = 0.0;
	double productMass = 0.0;
	int reactantCharge = 0;
	int productCharge = 0;
	auto validateSide = [&](const std::vector<OmniReactionTerm> &terms, int atomSign,
		double &mass, int &charge) -> bool {
		for (const auto &term : terms)
		{
			if (term.species >= species.size() || term.stoichiometricCoefficient <= 0)
				return false;
			const auto &item = species[term.species];
			if (item.id.empty() || !Finite(item.molarMassKgPerMol) || item.molarMassKgPerMol <= 0.0 ||
				item.elementalComposition.empty())
				return false;
			for (const auto &element : item.elementalComposition)
				if (element.symbol.empty() || element.count <= 0)
					return false;
			mass += SpeciesMoles(item, term.stoichiometricCoefficient);
			charge += item.chargeNumber * term.stoichiometricCoefficient;
			AccumulateAtoms(atomResidual, item, term.stoichiometricCoefficient, atomSign);
		}
		return true;
	};
	if (!validateSide(reaction.reactants, -1, reactantMass, reactantCharge) ||
		!validateSide(reaction.products, 1, productMass, productCharge))
	{
		result.reason = "reaction contains an invalid species or coefficient";
		return result;
	}
	for (const auto &[element, residual] : atomResidual)
	{
		if (residual != 0)
		{
			result.reason = "atom conservation failed for " + element;
			return result;
		}
	}
	result.chargeResidual = productCharge - reactantCharge;
	if (result.chargeResidual != 0)
	{
		result.reason = "charge conservation failed";
		return result;
	}
	result.molarMassResidualKgPerMol = productMass - reactantMass;
	const double scale = std::max({ reactantMass, productMass, 1.0 });
	if (std::abs(result.molarMassResidualKgPerMol) > 1.0e-12 * scale)
	{
		result.reason = "molar mass conservation failed";
		return result;
	}
	result.valid = true;
	return result;
}

OmniCarbonOxidationPlan OmniReactionRuntime::PlanCarbonOxidation(
	const OmniCarbonOxidationInput &input) const
{
	OmniCarbonOxidationPlan result;
	if (species.size() <= CarbonDioxideSpecies || reactions.empty() ||
		!Finite(input.carbonMassKg) || input.carbonMassKg < 0.0 ||
		!Finite(input.oxygenMassKg) || input.oxygenMassKg < 0.0 ||
		!Finite(input.temperatureK) || input.temperatureK <= 0.0 ||
		!Finite(input.timestepS) || input.timestepS < 0.0 ||
		!Finite(input.activeSurfaceFraction) || input.activeSurfaceFraction < 0.0 ||
		input.activeSurfaceFraction > 1.0)
	{
		return result;
	}
	const auto validation = ValidateReaction(0);
	if (!validation.valid)
		return result;

	const auto &reaction = reactions[0];
	const bool isCarbonOxidation =
		species[CarbonSpecies].id == "species.carbon" &&
		species[OxygenSpecies].id == "species.oxygen" &&
		species[CarbonDioxideSpecies].id == "species.carbon-dioxide" &&
		reaction.id == "reaction.carbon-oxidation" &&
		reaction.reactants.size() == 2 && reaction.products.size() == 1 &&
		reaction.reactants[0].species == CarbonSpecies && reaction.reactants[0].stoichiometricCoefficient == 1 &&
		reaction.reactants[1].species == OxygenSpecies && reaction.reactants[1].stoichiometricCoefficient == 1 &&
		reaction.products[0].species == CarbonDioxideSpecies && reaction.products[0].stoichiometricCoefficient == 1;
	if (!isCarbonOxidation)
		return result;
	const double carbonAvailableMol = input.carbonMassKg / species[CarbonSpecies].molarMassKgPerMol;
	const double oxygenAvailableMol = input.oxygenMassKg / species[OxygenSpecies].molarMassKgPerMol;
	const double exponent = -reaction.rate.activationEnergyJPerMol /
		(UniversalGasConstantJPerMolK * input.temperatureK);
	const double temperatureRatio = input.temperatureK / reaction.rate.referenceTemperatureK;
	const double ratePerS = reaction.rate.preExponentialPerS *
		std::pow(temperatureRatio, reaction.rate.temperatureExponent) * std::exp(exponent);
	if (!Finite(ratePerS) || ratePerS < 0.0)
		return result;
	const double conversion = -std::expm1(-ratePerS * input.timestepS);
	const double kineticExtent = oxygenAvailableMol *
		std::clamp(conversion * input.activeSurfaceFraction, 0.0, 1.0);
	result.extentMol = std::min({ kineticExtent, carbonAvailableMol, oxygenAvailableMol });
	result.limitedByCarbon = carbonAvailableMol <= std::min(kineticExtent, oxygenAvailableMol);
	result.limitedByOxygen = oxygenAvailableMol <= std::min(kineticExtent, carbonAvailableMol);
	result.carbonConsumedKg = result.extentMol * species[CarbonSpecies].molarMassKgPerMol;
	result.oxygenConsumedKg = result.extentMol * species[OxygenSpecies].molarMassKgPerMol;
	result.carbonDioxideProducedKg = result.extentMol * species[CarbonDioxideSpecies].molarMassKgPerMol;
	result.heatReleasedJ = -result.extentMol * reaction.enthalpyChangeJPerMol;
	result.massResidualKg = result.carbonDioxideProducedKg -
		(result.carbonConsumedKg + result.oxygenConsumedKg);
	result.carbonAtomResidualMol = result.extentMol * (
		AtomCount(species[CarbonDioxideSpecies], "C") - AtomCount(species[CarbonSpecies], "C") -
		AtomCount(species[OxygenSpecies], "C"));
	result.oxygenAtomResidualMol = result.extentMol * (
		AtomCount(species[CarbonDioxideSpecies], "O") - AtomCount(species[CarbonSpecies], "O") -
		AtomCount(species[OxygenSpecies], "O"));
	result.energyResidualJ = result.heatReleasedJ + result.extentMol * reaction.enthalpyChangeJPerMol;
	result.valid = Finite(result.extentMol) && Finite(result.heatReleasedJ) &&
		result.extentMol >= 0.0 && result.carbonConsumedKg <= input.carbonMassKg + 1.0e-18 &&
		result.oxygenConsumedKg <= input.oxygenMassKg + 1.0e-18 &&
		std::abs(result.massResidualKg) <= 1.0e-15 &&
		std::abs(result.carbonAtomResidualMol) <= 1.0e-15 &&
		std::abs(result.oxygenAtomResidualMol) <= 1.0e-15 &&
		std::abs(result.energyResidualJ) <= 1.0e-9;
	return result;
}
