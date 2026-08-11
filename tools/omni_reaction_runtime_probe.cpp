#include "simulation/OmniReactionRuntime.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
int Fail(const char *message)
{
	std::cerr << "FAIL: " << message << '\n';
	return 1;
}
}

int main()
{
	const auto runtime = OmniReactionRuntime::CarbonOxidationV1();
	const auto validation = runtime.ValidateReaction(0);
	if (!validation.valid || std::abs(validation.molarMassResidualKgPerMol) > 1.0e-15 ||
		validation.chargeResidual != 0)
		return Fail("carbon oxidation catalog contract");

	const OmniCarbonOxidationInput hot{ 0.0120107, 0.0319988, 1000.0, 1.0 / 60.0, 1.0 };
	const auto hotPlan = runtime.PlanCarbonOxidation(hot);
	if (!hotPlan.valid || !(hotPlan.extentMol > 0.0 && hotPlan.extentMol < 1.0) ||
		hotPlan.heatReleasedJ <= 0.0 || std::abs(hotPlan.massResidualKg) > 1.0e-15 ||
		std::abs(hotPlan.carbonAtomResidualMol) > 1.0e-15 ||
		std::abs(hotPlan.oxygenAtomResidualMol) > 1.0e-15 ||
		std::abs(hotPlan.energyResidualJ) > 1.0e-9)
		return Fail("hot finite-rate transaction");

	const auto roomPlan = runtime.PlanCarbonOxidation({
		hot.carbonMassKg, hot.oxygenMassKg, 293.15, hot.timestepS, 1.0 });
	if (!roomPlan.valid || !(roomPlan.extentMol < hotPlan.extentMol))
		return Fail("temperature-dependent kinetics");

	const auto lowOxygen = runtime.PlanCarbonOxidation({
		hot.carbonMassKg, hot.oxygenMassKg * 0.1, hot.temperatureK, hot.timestepS, 1.0 });
	if (!lowOxygen.valid || !(lowOxygen.extentMol < hotPlan.extentMol))
		return Fail("oxygen-dependent kinetics");

	const auto noSurface = runtime.PlanCarbonOxidation({
		hot.carbonMassKg, hot.oxygenMassKg, hot.temperatureK, hot.timestepS, 0.0 });
	if (!noSurface.valid || noSurface.extentMol != 0.0)
		return Fail("inactive surface contract");

	const auto carbonLimited = runtime.PlanCarbonOxidation({
		1.0e-12, hot.oxygenMassKg, 5000.0, 1000.0, 1.0 });
	if (!carbonLimited.valid || !carbonLimited.limitedByCarbon ||
		carbonLimited.carbonConsumedKg > 1.0e-12 + 1.0e-18)
		return Fail("carbon limiting contract");

	if (runtime.PlanCarbonOxidation({ -1.0, 1.0, 1000.0, 0.1, 1.0 }).valid)
		return Fail("invalid input rejection");

	bool rejectedUnbalanced = false;
	try
	{
		auto species = OmniReactionRuntime::CarbonOxidationSpeciesV1();
		auto reaction = OmniReactionRuntime::CarbonOxidationReactionV1();
		reaction.products[0].stoichiometricCoefficient = 2;
		OmniReactionRuntime invalid(std::move(species), { std::move(reaction) });
	}
	catch (const std::invalid_argument &)
	{
		rejectedUnbalanced = true;
	}
	if (!rejectedUnbalanced)
		return Fail("unbalanced reaction rejection");

	std::cout << "reaction=carbon_oxidation_v1\n"
		<< "catalog_valid=true\n"
		<< "hot_extent_mol=" << hotPlan.extentMol << '\n'
		<< "room_extent_mol=" << roomPlan.extentMol << '\n'
		<< "heat_released_j=" << hotPlan.heatReleasedJ << '\n'
		<< "mass_residual_kg=" << hotPlan.massResidualKg << '\n'
		<< "carbon_atom_residual_mol=" << hotPlan.carbonAtomResidualMol << '\n'
		<< "oxygen_atom_residual_mol=" << hotPlan.oxygenAtomResidualMol << '\n'
		<< "energy_residual_j=" << hotPlan.energyResidualJ << '\n'
		<< "unbalanced_rejected=true\n";
	return 0;
}
