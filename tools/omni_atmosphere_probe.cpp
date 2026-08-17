#include "simulation/OmniAtmosphere.h"
#include "simulation/OmniThermal.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace
{
int Fail(std::string_view message)
{
	std::cerr << "omni-atmosphere-probe: FAIL " << message << '\n';
	return 1;
}

bool Close(double left, double right, double absolute, double relative)
{
	return std::abs(left - right) <= absolute + relative * std::max(std::abs(left), std::abs(right));
}

bool IsNaNBits(double value)
{
	const auto bits = std::bit_cast<std::uint64_t>(value);
	return (bits & UINT64_C(0x7FF0000000000000)) == UINT64_C(0x7FF0000000000000) &&
		(bits & UINT64_C(0x000FFFFFFFFFFFFF)) != 0;
}

OmniAtmosphereConfig Config(std::size_t width, std::size_t height, OmniAtmosphereBoundary boundary = OmniAtmosphereBoundary::Sealed)
{
	OmniAtmosphereConfig config;
	config.width = width;
	config.height = height;
	config.boundary = boundary;
	config.execution = OmniAtmosphereExecution::ReferenceCompressible;
	return config;
}
}

int main()
{
	OmniAtmosphere uniform(Config(32, 16, OmniAtmosphereBoundary::Periodic));
	const auto initialUniform = uniform.State(4, 4);
	uniform.StepReference(1.0e-5);
	const auto finalUniform = uniform.State(4, 4);
	if (!Close(initialUniform.density, finalUniform.density, 1.0e-14, 1.0e-13) ||
		!Close(initialUniform.momentumX, finalUniform.momentumX, 1.0e-14, 1.0e-13) ||
		!Close(initialUniform.totalEnergy, finalUniform.totalEnergy, 1.0e-10, 1.0e-13))
	{
		return Fail("uniform conservative state drifted");
	}
	if (std::abs(uniform.Ledger().massResidualKg()) > 1.0e-15 ||
		std::abs(uniform.Ledger().momentumXResidual()) > 1.0e-15 ||
		std::abs(uniform.Ledger().momentumYResidual()) > 1.0e-15 ||
		std::abs(uniform.Ledger().energyResidualJ()) > 1.0e-11 ||
		uniform.Ledger().nonFiniteCells || uniform.Ledger().densityFloorHits ||
		uniform.Ledger().pressureFloorHits || uniform.Ledger().energyFloorHits)
	{
		return Fail("uniform ledger did not close");
	}

	OmniAtmosphere energyGuard(Config(8, 4));
	const auto energyGuardBefore = energyGuard.State(3, 2);
	const double energyGuardTotalBefore = energyGuard.TotalEnergyJ();
	const double overdrawJ = energyGuard.AvailableThermalEnergyJ(3, 2) + 1.0;
	bool overdrawRejected = false;
	try
	{
		energyGuard.AddEnergyDensity(
			3, 2, -overdrawJ / energyGuard.Config().scale.cellVolumeM3());
	}
	catch (const std::invalid_argument &)
	{
		overdrawRejected = true;
	}
	const auto energyGuardAfter = energyGuard.State(3, 2);
	if (!overdrawRejected || energyGuardAfter.density != energyGuardBefore.density ||
		energyGuardAfter.momentumX != energyGuardBefore.momentumX ||
		energyGuardAfter.momentumY != energyGuardBefore.momentumY ||
		energyGuardAfter.totalEnergy != energyGuardBefore.totalEnergy ||
		energyGuard.TotalEnergyJ() != energyGuardTotalBefore)
	{
		return Fail("negative energy overdraw did not fail transactionally");
	}

	// At very low density the pressure floor, rather than the caller's nominal
	// one-kelvin bound, is the tighter thermal-energy constraint.  Removing all
	// reported available energy must land on (and never below) that floor.
	OmniAtmosphere pressureEnergyGuard(Config(2, 2));
	std::vector<double> pressureFloorSpecies(pressureEnergyGuard.SpeciesCount(), 0.0);
	pressureFloorSpecies[OMNI_SPECIES_N2] = pressureEnergyGuard.Config().densityFloor;
	if (!pressureEnergyGuard.RestoreSerializedCell(
			0, 0, pressureFloorSpecies, 0.0, 0.0, 0.01, 0.0))
	{
		return Fail("could not construct the pressure-floor thermal-energy fixture");
	}
	const double pressureFloorAvailable = pressureEnergyGuard.AvailableThermalEnergyJ(0, 0, 1.0);
	pressureEnergyGuard.AddEnergyDensity(
		0, 0, -pressureFloorAvailable / pressureEnergyGuard.Config().scale.cellVolumeM3());
	if (!pressureEnergyGuard.Primitive(0, 0).finite ||
		pressureEnergyGuard.Primitive(0, 0).pressure + 1.0e-12 <
			pressureEnergyGuard.Config().pressureFloor)
	{
		return Fail("available thermal energy allowed pressure to fall below its floor");
	}

	auto internalFloorConfig = Config(1, 1);
	internalFloorConfig.internalEnergyFloor = 1.0e5;
	OmniAtmosphere internalFloorGuard(internalFloorConfig);
	std::vector<double> internalFloorSpecies(internalFloorGuard.SpeciesCount(), 0.0);
	for (std::size_t species = 0; species < internalFloorSpecies.size(); ++species)
		internalFloorSpecies[species] = internalFloorGuard.SpeciesMassDensity(0, 0, species);
	const auto internalFloorBefore = internalFloorGuard.State(0, 0);
	if (!OmniValidateSerializedAtmosphereCell(
			internalFloorGuard.Config(), internalFloorSpecies,
			internalFloorBefore.momentumX, internalFloorBefore.momentumY,
			internalFloorBefore.totalEnergy,
			internalFloorGuard.Primitive(0, 0).condensedWaterDensity, true))
	{
		return Fail("internal-energy-floor fixture was not initially serializable");
	}
	const double internalFloorAvailable = internalFloorGuard.AvailableThermalEnergyJ(0, 0, 1.0);
	if (!(internalFloorAvailable > 0.0))
		return Fail("internal-energy-floor fixture reported no removable energy");
	internalFloorGuard.AddEnergyDensity(
		0, 0, -0.999 * internalFloorAvailable /
			internalFloorGuard.Config().scale.cellVolumeM3());
	const auto internalFloorAfter = internalFloorGuard.State(0, 0);
	if (!OmniValidateSerializedAtmosphereCell(
			internalFloorGuard.Config(), internalFloorSpecies,
			internalFloorAfter.momentumX, internalFloorAfter.momentumY,
			internalFloorAfter.totalEnergy,
			internalFloorGuard.Primitive(0, 0).condensedWaterDensity, true))
	{
		return Fail("available thermal energy crossed the serializable internal-energy floor");
	}
	const double internalFloorDeficientEnergy =
		0.5 * internalFloorGuard.Config().internalEnergyFloor;
	if (internalFloorGuard.RestoreSerializedCell(
			0, 0, internalFloorSpecies, 0.0, 0.0,
			internalFloorDeficientEnergy, 0.0))
	{
		return Fail("serialized restore accepted energy below internal-energy floor");
	}

	OmniAtmosphere positiveOverflowGuard(Config(2, 2));
	const double hugeEnergyDensity = std::numeric_limits<double>::max() * 0.75;
	positiveOverflowGuard.AddEnergyDensity(0, 0, hugeEnergyDensity);
	const auto overflowBeforeRejectedSource = positiveOverflowGuard.State(0, 0);
	bool positiveOverflowRejected = false;
	try
	{
		positiveOverflowGuard.AddEnergyDensity(0, 0, hugeEnergyDensity);
	}
	catch (const std::invalid_argument &)
	{
		positiveOverflowRejected = true;
	}
	const auto overflowAfterRejectedSource = positiveOverflowGuard.State(0, 0);
	if (!positiveOverflowRejected ||
		overflowBeforeRejectedSource.totalEnergy != overflowAfterRejectedSource.totalEnergy ||
		!std::isfinite(overflowAfterRejectedSource.totalEnergy))
	{
		return Fail("positive energy overflow was not rejected transactionally");
	}

	std::vector<double> latentDeficientSpecies(pressureEnergyGuard.SpeciesCount(), 0.0);
	latentDeficientSpecies[OMNI_SPECIES_N2] = 1.0;
	latentDeficientSpecies[OMNI_SPECIES_H2O] = 0.1;
	const double latentDeficientPositiveEnergy =
		0.5 * latentDeficientSpecies[OMNI_SPECIES_H2O] *
		OmniThermal::LatentHeatVaporizationJPerKg;
	if (OmniValidateSerializedAtmosphereCell(
			pressureEnergyGuard.Config(), latentDeficientSpecies, 0.0, 0.0,
			latentDeficientPositiveEnergy, 0.0, true) ||
		pressureEnergyGuard.RestoreSerializedCell(
			1, 0, latentDeficientSpecies, 0.0, 0.0,
			latentDeficientPositiveEnergy, 0.0))
	{
		return Fail("positive but latent-energy-deficient serialized cell was accepted");
	}

	OmniAtmosphere latentGuard(Config(8, 4));
	const auto latentBefore = latentGuard.State(3, 2);
	const double latentWaterBefore = latentGuard.SpeciesMassDensity(3, 2, OMNI_SPECIES_H2O);
	const double transferMassKg =
		latentGuard.Config().referenceDensity * latentGuard.Config().scale.cellVolumeM3();
	const auto &waterSpecies = latentGuard.SpeciesDefinition(OMNI_SPECIES_H2O);
	const double automaticVaporEnergyJ = transferMassKg *
		OmniThermal::WaterVaporSpecificEnergyJPerKg(
			300.0, waterSpecies.specificHeatCpJKgK, waterSpecies.molarMassKgPerMol);
	const double liquidEnergyJ =
		transferMassKg * OmniThermal::WaterSpecificEnthalpyJPerKg(300.0);
	std::vector<double> insufficientTransfer(latentGuard.SpeciesCount(), 0.0);
	insufficientTransfer[OMNI_SPECIES_H2O] = transferMassKg;
	if (latentGuard.ApplyReactionSpeciesTransfer(3, 2, insufficientTransfer,
			liquidEnergyJ - automaticVaporEnergyJ, 0.0, 0.0) ||
		latentGuard.State(3, 2).density != latentBefore.density ||
		latentGuard.State(3, 2).totalEnergy != latentBefore.totalEnergy ||
		latentGuard.SpeciesMassDensity(3, 2, OMNI_SPECIES_H2O) != latentWaterBefore)
	{
		return Fail("latent-energy-deficient species transfer did not fail transactionally");
	}
	OmniAtmosphereReactionTransfer rejectedTransfer;
	rejectedTransfer.committed = true;
	rejectedTransfer.gasMassDeltaKg = 1.0;
	std::vector<double> wrongSizedTransfer(latentGuard.SpeciesCount() - 1, 0.0);
	if (latentGuard.ApplyReactionSpeciesTransfer(
			3, 2, wrongSizedTransfer, 0.0, 0.0, 0.0, &rejectedTransfer) ||
		rejectedTransfer.committed || rejectedTransfer.gasMassDeltaKg != 0.0 ||
		rejectedTransfer.totalEnergyDeltaJ != 0.0)
	{
		return Fail("rejected reaction transfer left stale caller result fields");
	}

	// A reaction transfer into the default dry reference atmosphere must arm
	// water phase equilibrium.  At this loading the injected vapour is strongly
	// supersaturated, so a reference step must create condensed water.
	OmniAtmosphere reactionPhase(Config(8, 4));
	std::vector<double> waterTransfer(reactionPhase.SpeciesCount(), 0.0);
	waterTransfer[OMNI_SPECIES_H2O] =
		0.25 * reactionPhase.Config().referenceDensity * reactionPhase.Config().scale.cellVolumeM3();
	if (!reactionPhase.ApplyReactionSpeciesTransfer(3, 2, waterTransfer, 0.0, 0.0, 0.0) ||
		reactionPhase.TotalCondensedWaterMassKg() != 0.0)
	{
		return Fail("could not construct dry-atmosphere reaction water transfer");
	}
	reactionPhase.StepReference(1.0e-6);
	if (!(reactionPhase.TotalCondensedWaterMassKg() > 0.0))
	{
		return Fail("reaction water transfer did not activate phase equilibrium");
	}

	auto rejectsTransportConfiguration = [](OmniAtmosphereConfig invalidConfig) {
		try
		{
			OmniAtmosphere invalid(invalidConfig);
			(void)invalid;
			return false;
		}
		catch (const std::invalid_argument &)
		{
			return true;
		}
	};
	auto invalidTransportConfig = Config(1, 1);
	invalidTransportConfig.species[OMNI_SPECIES_N2].thermalConductivityWMK = -1.0;
	if (!rejectsTransportConfiguration(invalidTransportConfig))
		return Fail("negative thermal conductivity was accepted");
	invalidTransportConfig = Config(1, 1);
	invalidTransportConfig.species[OMNI_SPECIES_N2].thermalConductivityWMK =
		std::numeric_limits<double>::infinity();
	if (!rejectsTransportConfiguration(invalidTransportConfig))
		return Fail("non-finite thermal conductivity was accepted");
	invalidTransportConfig = Config(1, 1);
	invalidTransportConfig.species[OMNI_SPECIES_N2].diffusionCoefficientM2S = -1.0;
	if (!rejectsTransportConfiguration(invalidTransportConfig))
		return Fail("negative species diffusivity was accepted");
	invalidTransportConfig = Config(1, 1);
	invalidTransportConfig.species[OMNI_SPECIES_N2].diffusionCoefficientM2S =
		std::numeric_limits<double>::quiet_NaN();
	if (!rejectsTransportConfiguration(invalidTransportConfig))
		return Fail("non-finite species diffusivity was accepted");

	auto expectedEquilibriumWaterDensity = [](const OmniAtmosphere &atmosphere) {
		const auto primitive = atmosphere.Primitive(0, 0);
		const auto &water = atmosphere.SpeciesDefinition(OMNI_SPECIES_H2O);
		const double waterGasConstant = 8.31446261815324 / water.molarMassKgPerMol;
		const double totalWaterDensity =
			atmosphere.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O) +
			primitive.condensedWaterDensity;
		return std::clamp(
			OmniThermal::SaturationPressurePa(primitive.temperature) /
				(waterGasConstant * primitive.temperature),
			0.0,
			totalWaterDensity);
	};

	auto movingPhaseConfig = Config(1, 1, OmniAtmosphereBoundary::Periodic);
	movingPhaseConfig.speciesDiffusion = false;
	movingPhaseConfig.thermalConduction = false;
	OmniAtmosphere movingPhase(movingPhaseConfig);
	movingPhase.ResetUniform(
		movingPhase.Config().referenceDensity, 320.0, 35.0, -12.0);
	movingPhase.SetCondensedWaterDensity(0, 0, 0.5);
	const auto movingBefore = movingPhase.Primitive(0, 0);
	const double movingEnergyBefore = movingPhase.TotalEnergyJ();
	const double movingWaterBefore =
		movingPhase.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O) +
		movingBefore.condensedWaterDensity;
	movingPhase.StepReference(1.0e-6);
	const auto movingAfter = movingPhase.Primitive(0, 0);
	const double movingExpectedVapour = expectedEquilibriumWaterDensity(movingPhase);
	const double movingWaterAfter =
		movingPhase.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O) +
		movingAfter.condensedWaterDensity;
	if (!movingAfter.finite || !(movingAfter.condensedWaterDensity > 0.0) ||
		!(movingPhase.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O) > 0.0) ||
		!Close(movingPhase.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O),
			movingExpectedVapour, 1.0e-10, 1.0e-9) ||
		!Close(movingAfter.velocityX, movingBefore.velocityX, 1.0e-11, 1.0e-12) ||
		!Close(movingAfter.velocityY, movingBefore.velocityY, 1.0e-11, 1.0e-12) ||
		!Close(movingWaterAfter, movingWaterBefore, 1.0e-12, 1.0e-12) ||
		!Close(movingPhase.TotalEnergyJ(), movingEnergyBefore, 1.0e-10, 1.0e-12))
	{
		std::cerr << "moving phase values: finite=" << movingAfter.finite
			<< " vapour=" << movingPhase.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O)
			<< " expected_vapour=" << movingExpectedVapour
			<< " condensed=" << movingAfter.condensedWaterDensity
			<< " velocity_before=" << movingBefore.velocityX << ',' << movingBefore.velocityY
			<< " velocity_after=" << movingAfter.velocityX << ',' << movingAfter.velocityY
			<< " water_before=" << movingWaterBefore
			<< " water_after=" << movingWaterAfter
			<< " energy_before=" << movingEnergyBefore
			<< " energy_after=" << movingPhase.TotalEnergyJ() << '\n';
		return Fail("moving water phase equilibrium did not conserve velocity, water, and energy");
	}

	auto phaseFloorConfig = Config(1, 1);
	phaseFloorConfig.speciesDiffusion = false;
	phaseFloorConfig.thermalConduction = false;
	phaseFloorConfig.pressureFloor = 120000.0;
	OmniAtmosphere phaseAfterFloor(phaseFloorConfig);
	phaseAfterFloor.ResetUniform(
		phaseAfterFloor.Config().referenceDensity, 320.0, 18.0, 0.0);
	phaseAfterFloor.SetCondensedWaterDensity(0, 0, 1.0);
	const auto phaseFloorState = phaseAfterFloor.State(0, 0);
	phaseAfterFloor.SetCell(0, 0, {
		phaseFloorState.density,
		phaseFloorState.momentumX,
		phaseFloorState.momentumY,
		1.0,
	});
	phaseAfterFloor.StepReference(1.0e-6);
	const auto phaseFloorPrimitive = phaseAfterFloor.Primitive(0, 0);
	const double phaseFloorExpectedVapour =
		expectedEquilibriumWaterDensity(phaseAfterFloor);
	const double phaseFloorEnergyBeforeSaveCheck = phaseAfterFloor.State(0, 0).totalEnergy;
	if (!phaseFloorPrimitive.finite || !phaseAfterFloor.Ledger().energyFloorHits ||
		!phaseAfterFloor.Ledger().pressureFloorHits ||
		!Close(phaseAfterFloor.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O),
			phaseFloorExpectedVapour, 1.0e-10, 1.0e-9) ||
		!phaseAfterFloor.EnsureSerializableState() ||
		phaseAfterFloor.State(0, 0).totalEnergy != phaseFloorEnergyBeforeSaveCheck)
	{
		return Fail("phase equilibrium was not preserved after the serializable energy floor");
	}

	auto seamDiffusionConfig = Config(3, 1, OmniAtmosphereBoundary::Periodic);
	seamDiffusionConfig.thermalConduction = false;
	seamDiffusionConfig.waterPhaseEquilibrium = false;
	for (auto &species : seamDiffusionConfig.species)
		species.diffusionCoefficientM2S = 0.0;
	seamDiffusionConfig.species[OMNI_SPECIES_N2].diffusionCoefficientM2S = 0.05;
	seamDiffusionConfig.species[OMNI_SPECIES_O2].diffusionCoefficientM2S = 0.05;
	seamDiffusionConfig.species[OMNI_SPECIES_O2].molarMassKgPerMol =
		seamDiffusionConfig.species[OMNI_SPECIES_N2].molarMassKgPerMol;
	seamDiffusionConfig.species[OMNI_SPECIES_O2].specificHeatCpJKgK =
		seamDiffusionConfig.species[OMNI_SPECIES_N2].specificHeatCpJKgK;
	auto seamControlConfig = seamDiffusionConfig;
	seamControlConfig.speciesDiffusion = false;
	OmniAtmosphere seamDiffusion(seamDiffusionConfig);
	OmniAtmosphere seamControl(seamControlConfig);
	std::vector<double> seamNitrogen(seamDiffusion.SpeciesCount(), 0.0);
	std::vector<double> seamOxygen(seamDiffusion.SpeciesCount(), 0.0);
	seamNitrogen[OMNI_SPECIES_N2] = 1.0;
	seamOxygen[OMNI_SPECIES_O2] = 1.0;
	for (auto *atmosphere : { &seamDiffusion, &seamControl })
	{
		atmosphere->SetBlocked(1, 0, true);
		atmosphere->SetSpeciesMassFractions(0, 0, seamNitrogen);
		atmosphere->SetSpeciesMassFractions(2, 0, seamOxygen);
	}
	const double seamNitrogenBefore =
		seamDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2);
	const double seamNitrogenTotalBefore =
		seamDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_N2);
	seamDiffusion.StepReference(1.0e-6);
	seamControl.StepReference(1.0e-6);
	if (!(seamDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2) <
			seamControl.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2) - 1.0e-8) ||
		!Close(seamControl.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2),
			seamNitrogenBefore, 1.0e-12, 1.0e-12) ||
		!Close(seamDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_N2),
			seamNitrogenTotalBefore, 1.0e-15, 1.0e-12) ||
		std::abs(seamDiffusion.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("periodic seam-only molecular diffusion was skipped or non-conservative");
	}

	auto transportLimitConfig = seamDiffusionConfig;
	transportLimitConfig.width = 2;
	transportLimitConfig.boundary = OmniAtmosphereBoundary::Sealed;
	transportLimitConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	transportLimitConfig.maximumRuntimeSubsteps = 4;
	OmniAtmosphere transportLimited(transportLimitConfig);
	transportLimited.SetSpeciesMassFractions(0, 0, seamNitrogen);
	transportLimited.SetSpeciesMassFractions(1, 0, seamOxygen);
	transportLimited.Step();
	transportLimited.Step();
	if (transportLimited.Ledger().acousticRoute ||
		!transportLimited.Ledger().timestepLimited ||
		transportLimited.Ledger().substeps != transportLimitConfig.maximumRuntimeSubsteps ||
		!(transportLimited.Ledger().advancedTimestepS <
			transportLimited.Ledger().requestedTimestepS) ||
		transportLimited.NonFiniteStateCells() != 0)
	{
		return Fail("explicit molecular diffusion did not constrain the runtime timestep");
	}

	// Reproduce the long-run failure mode in a bounded two-cell fixture. A
	// near-floor pure-water cell counter-diffuses into a near-floor dry cell;
	// after condensation, every authoritative cell must still satisfy the exact
	// OPS v3 predicate. Moving species density without its enthalpy makes this
	// fixture latent/sensible-energy deficient.
	auto diffusionConfig = Config(2, 1);
	diffusionConfig.thermalConduction = false;
	for (auto &species : diffusionConfig.species)
		species.diffusionCoefficientM2S = 0.1;
	OmniAtmosphere waterDiffusion(diffusionConfig);
	std::vector<double> pureWater(waterDiffusion.SpeciesCount(), 0.0);
	std::vector<double> pureNitrogen(waterDiffusion.SpeciesCount(), 0.0);
	pureWater[OMNI_SPECIES_N2] = 0.5;
	pureWater[OMNI_SPECIES_H2O] = 0.5;
	pureNitrogen[OMNI_SPECIES_N2] = 1.0;
	waterDiffusion.SetSpeciesMassFractions(0, 0, pureWater);
	waterDiffusion.SetSpeciesMassFractions(1, 0, pureNitrogen);
	for (std::size_t x = 0; x < waterDiffusion.Width(); ++x)
	{
		const double removable = waterDiffusion.AvailableThermalEnergyJ(x, 0, 100.0);
		waterDiffusion.AddEnergyDensity(
			x, 0, -removable / waterDiffusion.Config().scale.cellVolumeM3());
	}
	const double waterLeftBefore =
		waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O);
	const double waterRightBefore =
		waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_H2O);
	const double nitrogenLeftBefore =
		waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2);
	const double nitrogenRightBefore =
		waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_N2);
	const auto diffusionLeftPrimitiveBefore = waterDiffusion.Primitive(0, 0);
	const auto diffusionRightPrimitiveBefore = waterDiffusion.Primitive(1, 0);
	waterDiffusion.StepReference(1.0e-6);
	if (!(waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O) < waterLeftBefore) ||
		!(waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_H2O) > waterRightBefore) ||
		!(waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2) > nitrogenLeftBefore) ||
		!(waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_N2) < nitrogenRightBefore))
	{
		std::cerr << "counter-diffusion values: h2o_left="
			<< waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_H2O)
			<< " h2o_left_before=" << waterLeftBefore
			<< " h2o_right=" << waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_H2O)
			<< " h2o_right_before=" << waterRightBefore
			<< " n2_left=" << waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2)
			<< " n2_left_before=" << nitrogenLeftBefore
			<< " n2_right=" << waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_N2)
			<< " n2_right_before=" << nitrogenRightBefore
			<< " condensed_left=" << waterDiffusion.Primitive(0, 0).condensedWaterDensity
			<< " condensed_right=" << waterDiffusion.Primitive(1, 0).condensedWaterDensity
			<< " n2_left_delta="
			<< (waterDiffusion.SpeciesMassDensity(0, 0, OMNI_SPECIES_N2) - nitrogenLeftBefore)
			<< " n2_right_delta="
			<< (waterDiffusion.SpeciesMassDensity(1, 0, OMNI_SPECIES_N2) - nitrogenRightBefore)
			<< " diffusion_enabled=" << waterDiffusion.Config().speciesDiffusion
			<< " d_n2=" << waterDiffusion.Config().species[OMNI_SPECIES_N2].diffusionCoefficientM2S
			<< " d_h2o=" << waterDiffusion.Config().species[OMNI_SPECIES_H2O].diffusionCoefficientM2S
			<< " left_finite=" << diffusionLeftPrimitiveBefore.finite
			<< " left_temp=" << diffusionLeftPrimitiveBefore.temperature
			<< " right_finite=" << diffusionRightPrimitiveBefore.finite
			<< " right_temp=" << diffusionRightPrimitiveBefore.temperature
			<< '\n';
		return Fail("water/nitrogen counter-diffusion did not move both species");
	}
	for (std::size_t x = 0; x < waterDiffusion.Width(); ++x)
	{
		std::vector<double> speciesState(waterDiffusion.SpeciesCount(), 0.0);
		for (std::size_t species = 0; species < speciesState.size(); ++species)
			speciesState[species] = waterDiffusion.SpeciesMassDensity(x, 0, species);
		const auto &cell = waterDiffusion.State(x, 0);
		if (!OmniValidateSerializedAtmosphereCell(
				waterDiffusion.Config(), speciesState,
				cell.momentumX, cell.momentumY, cell.totalEnergy,
				waterDiffusion.Primitive(x, 0).condensedWaterDensity, true))
		{
			std::cerr << "water diffusion invalid cell x=" << x
				<< " density=" << cell.density
				<< " momentum_x=" << cell.momentumX
				<< " momentum_y=" << cell.momentumY
				<< " total_energy=" << cell.totalEnergy
				<< " condensed=" << waterDiffusion.Primitive(x, 0).condensedWaterDensity
				<< " n2=" << speciesState[OMNI_SPECIES_N2]
				<< " h2o=" << speciesState[OMNI_SPECIES_H2O] << '\n';
			return Fail("water diffusion and condensation produced an unserializable cell");
		}
	}
	if (waterDiffusion.NonFiniteStateCells() != 0)
		return Fail("strict runtime state metric missed water diffusion serializability");
	if (waterDiffusion.Ledger().energyFloorHits != 0 ||
		waterDiffusion.Ledger().numericalEnergyCorrectionJ != 0.0)
	{
		return Fail("species enthalpy diffusion required a hidden energy-floor correction");
	}

	auto floorBoundaryConfig = diffusionConfig;
	floorBoundaryConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	floorBoundaryConfig.waterPhaseEquilibrium = false;
	floorBoundaryConfig.species[OMNI_SPECIES_H2O].molarMassKgPerMol =
		floorBoundaryConfig.species[OMNI_SPECIES_N2].molarMassKgPerMol;
	floorBoundaryConfig.species[OMNI_SPECIES_H2O].specificHeatCpJKgK =
		floorBoundaryConfig.species[OMNI_SPECIES_N2].specificHeatCpJKgK;
	OmniAtmosphere floorBoundaryDiffusion(floorBoundaryConfig);
	floorBoundaryDiffusion.SetSpeciesMassFractions(0, 0, pureWater);
	floorBoundaryDiffusion.SetSpeciesMassFractions(1, 0, pureNitrogen);
	// Clear setter-driven acoustic work with a negligible reference interval.
	// The following runtime step then isolates molecular diffusion.
	floorBoundaryDiffusion.StepReference(1.0e-20);
	for (std::size_t x = 0; x < floorBoundaryDiffusion.Width(); ++x)
	{
		std::vector<double> speciesState(floorBoundaryDiffusion.SpeciesCount(), 0.0);
		for (std::size_t species = 0; species < speciesState.size(); ++species)
			speciesState[species] = floorBoundaryDiffusion.SpeciesMassDensity(x, 0, species);
		const auto initialState = floorBoundaryDiffusion.State(x, 0);
		double canonicalEnergy = 1.0;
		if (!OmniMigrateLegacySerializedAtmosphereCellV2(
				floorBoundaryDiffusion.Config(), speciesState,
				initialState.momentumX, initialState.momentumY, canonicalEnergy,
				floorBoundaryDiffusion.Primitive(x, 0).condensedWaterDensity, true))
		{
			return Fail("could not calculate one-kelvin diffusion energy floor");
		}
		const_cast<OmniAtmosphereConservative &>(
			floorBoundaryDiffusion.State(x, 0)).totalEnergy = canonicalEnergy;
	}
	if (!floorBoundaryDiffusion.EnsureSerializableState())
		return Fail("could not establish serializable one-kelvin diffusion fixture");
	const double floorBoundaryEnergyBefore = floorBoundaryDiffusion.TotalEnergyJ();
	const double floorBoundaryWaterBefore =
		floorBoundaryDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_H2O);
	const double floorBoundaryNitrogenBefore =
		floorBoundaryDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_N2);
	floorBoundaryDiffusion.Step();
	if (floorBoundaryDiffusion.Ledger().energyFloorHits != 0 ||
		floorBoundaryDiffusion.Ledger().numericalEnergyCorrectionJ != 0.0 ||
		floorBoundaryDiffusion.Ledger().acousticRoute ||
		floorBoundaryDiffusion.NonFiniteStateCells() != 0 ||
		!Close(floorBoundaryDiffusion.TotalEnergyJ(), floorBoundaryEnergyBefore,
			1.0e-14, 1.0e-12) ||
		!Close(floorBoundaryDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_H2O),
			floorBoundaryWaterBefore, 1.0e-18, 1.0e-12) ||
		!Close(floorBoundaryDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_N2),
			floorBoundaryNitrogenBefore, 1.0e-18, 1.0e-12))
	{
		std::cerr << "one-kelvin diffusion values: floor_hits="
			<< floorBoundaryDiffusion.Ledger().energyFloorHits
			<< " correction_j=" << floorBoundaryDiffusion.Ledger().numericalEnergyCorrectionJ
			<< " acoustic=" << floorBoundaryDiffusion.Ledger().acousticRoute
			<< " substeps=" << floorBoundaryDiffusion.Ledger().substeps
			<< " advanced=" << floorBoundaryDiffusion.Ledger().advancedTimestepS
			<< " nonfinite=" << floorBoundaryDiffusion.NonFiniteStateCells()
			<< " energy_before=" << floorBoundaryEnergyBefore
			<< " energy_after=" << floorBoundaryDiffusion.TotalEnergyJ()
			<< " water_before=" << floorBoundaryWaterBefore
			<< " water_after=" << floorBoundaryDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_H2O)
			<< " nitrogen_before=" << floorBoundaryNitrogenBefore
			<< " nitrogen_after=" << floorBoundaryDiffusion.TotalSpeciesMassKg(OMNI_SPECIES_N2)
			<< " temperature_left=" << floorBoundaryDiffusion.Primitive(0, 0).temperature
			<< " temperature_right=" << floorBoundaryDiffusion.Primitive(1, 0).temperature
			<< '\n';
		return Fail("one-kelvin species diffusion crossed the serializable energy floor");
	}

	// External legacy/coupling writes can leave a finite but one-ulp-below-floor
	// state after the solver has finished. The save boundary may canonicalize
	// that bounded rounding error, but must not heal gross or non-finite state.
	OmniAtmosphere postCoupling(Config(1, 1));
	postCoupling.SetCondensedWaterDensity(0, 0, 1000.0);
	const auto postCouplingState = postCoupling.State(0, 0);
	std::vector<double> postCouplingSpecies(postCoupling.SpeciesCount(), 0.0);
	for (std::size_t species = 0; species < postCouplingSpecies.size(); ++species)
		postCouplingSpecies[species] = postCoupling.SpeciesMassDensity(0, 0, species);
	double canonicalEnergy = 1.0;
	if (!OmniMigrateLegacySerializedAtmosphereCellV2(
			postCoupling.Config(), postCouplingSpecies,
			postCouplingState.momentumX, postCouplingState.momentumY,
			canonicalEnergy, postCoupling.Primitive(0, 0).condensedWaterDensity, true))
	{
		return Fail("could not establish canonical post-coupling floor fixture");
	}
	postCoupling.SetCell(0, 0, {
		postCouplingState.density,
		postCouplingState.momentumX,
		postCouplingState.momentumY,
		std::nextafter(canonicalEnergy, -std::numeric_limits<double>::infinity()),
	});
	for (std::size_t species = 0; species < postCouplingSpecies.size(); ++species)
		postCouplingSpecies[species] = postCoupling.SpeciesMassDensity(0, 0, species);
	const auto postCouplingPrimitive = postCoupling.Primitive(0, 0);
	if (OmniValidateSerializedAtmosphereCell(
			postCoupling.Config(), postCouplingSpecies,
			postCoupling.State(0, 0).momentumX, postCoupling.State(0, 0).momentumY,
			postCoupling.State(0, 0).totalEnergy,
			postCouplingPrimitive.condensedWaterDensity, true))
	{
		return Fail("post-coupling floor fixture was unexpectedly valid before repair");
	}
	if (!postCoupling.EnsureSerializableState() ||
		!OmniValidateSerializedAtmosphereCell(
			postCoupling.Config(), postCouplingSpecies,
			postCoupling.State(0, 0).momentumX, postCoupling.State(0, 0).momentumY,
			postCoupling.State(0, 0).totalEnergy,
			postCoupling.Primitive(0, 0).condensedWaterDensity, true) ||
		postCoupling.NonFiniteStateCells() != 0)
	{
		return Fail("post-coupling serializability repair failed");
	}

	OmniAtmosphere grossDeficit(Config(1, 1));
	grossDeficit.SetCondensedWaterDensity(0, 0, 1000.0);
	const auto grossState = grossDeficit.State(0, 0);
	grossDeficit.SetCell(0, 0, {
		grossState.density, grossState.momentumX, grossState.momentumY, 1.0 });
	if (grossDeficit.EnsureSerializableState() || grossDeficit.NonFiniteStateCells() == 0)
		return Fail("save-boundary repair hid a gross finite energy deficit");

	OmniAtmosphere nonFiniteDeficit(Config(1, 1));
	auto &nonFiniteState = const_cast<OmniAtmosphereConservative &>(
		nonFiniteDeficit.State(0, 0));
	nonFiniteState.totalEnergy = std::numeric_limits<double>::quiet_NaN();
	if (nonFiniteDeficit.EnsureSerializableState() ||
		std::isfinite(nonFiniteDeficit.State(0, 0).totalEnergy) ||
		nonFiniteDeficit.NonFiniteStateCells() == 0)
	{
		return Fail("save-boundary repair hid non-finite energy");
	}

	OmniAtmosphere corruptRuntimeDensity(Config(1, 1));
	std::vector<double> validSerializedSpecies(corruptRuntimeDensity.SpeciesCount(), 0.0);
	for (std::size_t species = 0; species < validSerializedSpecies.size(); ++species)
		validSerializedSpecies[species] =
			corruptRuntimeDensity.SpeciesMassDensity(0, 0, species);
	const auto validSerializedState = corruptRuntimeDensity.State(0, 0);
	if (!OmniValidateSerializedAtmosphereCell(
			corruptRuntimeDensity.Config(), validSerializedSpecies,
			validSerializedState.momentumX, validSerializedState.momentumY,
			validSerializedState.totalEnergy,
			corruptRuntimeDensity.Primitive(0, 0).condensedWaterDensity, true))
	{
		return Fail("runtime-density negative fixture did not start serializable");
	}
	const_cast<OmniAtmosphereConservative &>(
		corruptRuntimeDensity.State(0, 0)).density =
		std::numeric_limits<double>::quiet_NaN();
	if (corruptRuntimeDensity.EnsureSerializableState() ||
		corruptRuntimeDensity.NonFiniteStateCells() == 0 ||
		!IsNaNBits(corruptRuntimeDensity.State(0, 0).density))
		return Fail("runtime health metric ignored a non-finite live density");

	OmniAtmosphere heating(Config(32, 16));
	const double centrePressureBefore = heating.Primitive(16, 8).pressure;
	heating.AddEnergyDensity(16, 8, 25000.0);
	const double centrePressureAfterSource = heating.Primitive(16, 8).pressure;
	if (!(centrePressureAfterSource > centrePressureBefore))
		return Fail("sealed heating did not raise pressure");
	heating.StepReference(2.0e-5);
	if (!(heating.Primitive(15, 8).pressure > centrePressureBefore) ||
		std::abs(heating.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(heating.Ledger().energyResidualJ()) > 1.0e-10 ||
		heating.Ledger().nonFiniteCells)
	{
		return Fail("pressure pulse or sealed-heating ledger failed");
	}

	OmniAtmosphere vacuum(Config(32, 8));
	vacuum.ResetVacuum(1.0e-8, 293.15);
	const auto centre = vacuum.State(16, 4);
	vacuum.SetCell(16, 4, {
		centre.density * 100.0,
		0.0,
		0.0,
		centre.totalEnergy * 100.0,
	});
	vacuum.StepReference(1.0e-6);
	if (!(vacuum.MinimumDensity() > 0.0) || !(vacuum.MinimumPressure() > 0.0) ||
		vacuum.Ledger().nonFiniteCells || std::abs(vacuum.Ledger().massResidualKg()) > 1.0e-16 ||
		std::abs(vacuum.Ledger().energyResidualJ()) > 1.0e-14 ||
		!vacuum.Ledger().pressureFloorHits || !vacuum.Ledger().energyFloorHits ||
		!(vacuum.Ledger().numericalEnergyCorrectionJ > 0.0))
	{
		return Fail("near-vacuum positivity, correction accounting, or conservation failed");
	}

	OmniAtmosphere leak(Config(24, 12, OmniAtmosphereBoundary::Open));
	leak.ResetUniform(2.0, 350.0);
	const double leakMassBefore = leak.TotalMassKg();
	for (int step = 0; step < 12; ++step)
		leak.StepReference(2.0e-6);
	const double leakMassAfter = leak.TotalMassKg();
	if (!(leakMassAfter < leakMassBefore) || !(leak.Ledger().boundaryMassOutKg > 0.0) ||
		std::abs(leak.Ledger().massResidualKg()) > 1.0e-13 ||
		std::abs(leak.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(leak.Ledger().momentumYResidual()) > 1.0e-12 || leak.Ledger().nonFiniteCells)
	{
		return Fail("open-boundary leak did not close its ledger");
	}

	OmniAtmosphere wall(Config(20, 10));
	for (std::size_t y = 0; y < wall.Height(); ++y)
		wall.SetBlocked(10, y, true);
	const double wallMassBefore = wall.TotalMassKg();
	wall.AddEnergyDensity(8, 5, 100000.0);
	wall.StepReference(1.0e-5);
	if (std::abs(wall.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(wall.TotalMassKg() - wallMassBefore) > 1.0e-14 ||
		std::abs(wall.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(wall.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(wall.Ledger().energyResidualJ()) > 1.0e-10 ||
		wall.Ledger().nonFiniteCells)
	{
		return Fail("sealed wall conservation failed");
	}

	OmniAtmosphere blockedEdge(Config(20, 10));
	const auto asymmetricFrozen = blockedEdge.State(0, 4);
	blockedEdge.SetCell(0, 4, {
		asymmetricFrozen.density,
		asymmetricFrozen.density * 0.25,
		0.0,
		asymmetricFrozen.totalEnergy + 0.5 * asymmetricFrozen.density * 0.25 * 0.25,
	});
	for (std::size_t x = 0; x < blockedEdge.Width(); ++x)
	{
		blockedEdge.SetBlocked(x, 0, true);
		blockedEdge.SetBlocked(x, blockedEdge.Height() - 1, true);
	}
	for (std::size_t y = 1; y + 1 < blockedEdge.Height(); ++y)
	{
		blockedEdge.SetBlocked(0, y, true);
		blockedEdge.SetBlocked(blockedEdge.Width() - 1, y, true);
	}
	blockedEdge.AddEnergyDensity(2, 4, 75000.0);
	blockedEdge.StepReference(1.0e-5);
	if (std::abs(blockedEdge.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(blockedEdge.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(blockedEdge.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(blockedEdge.Ledger().energyResidualJ()) > 1.0e-10 ||
		blockedEdge.Ledger().nonFiniteCells)
	{
		return Fail("blocked outer wall added a phantom external-boundary impulse");
	}

	OmniAtmosphere runtime(Config(153, 96, OmniAtmosphereBoundary::Open));
	runtime.SetExecutionMode(OmniAtmosphereExecution::RuntimeLowMach);
	const auto runtimeInitialCorner = runtime.State(0, 0);
	runtime.Step();
	if (runtime.Ledger().timestepLimited || runtime.Ledger().substeps != 1 ||
		!Close(runtime.Ledger().advancedTimestepS, runtime.Ledger().requestedTimestepS, 1.0e-15, 1.0e-13) ||
		std::abs(runtime.Ledger().massResidualKg()) > 1.0e-14 || runtime.Ledger().nonFiniteCells ||
		!Close(runtime.State(0, 0).density, runtimeInitialCorner.density, 1.0e-14, 1.0e-13) ||
		!Close(runtime.State(0, 0).momentumX, 0.0, 1.0e-12, 0.0) ||
		!Close(runtime.State(0, 0).momentumY, 0.0, 1.0e-12, 0.0))
	{
		return Fail("quiet runtime low-Mach path failed");
	}
	constexpr int RuntimeBenchmarkTicks = 64;
	const auto benchmarkStart = std::chrono::steady_clock::now();
	for (int tick = 0; tick < RuntimeBenchmarkTicks; ++tick)
		runtime.Step();
	const auto benchmarkEnd = std::chrono::steady_clock::now();
	const double runtimeMillisecondsPerTick =
		std::chrono::duration<double, std::milli>(benchmarkEnd - benchmarkStart).count() /
		static_cast<double>(RuntimeBenchmarkTicks);

	auto limitedConfig = Config(16, 8);
	limitedConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	limitedConfig.maximumRuntimeSubsteps = 2;
	OmniAtmosphere limited(limitedConfig);
	limited.AddEnergyDensity(8, 4, 1.0e8);
	limited.Step();
	if (!limited.Ledger().timestepLimited ||
		!limited.Ledger().acousticRoute ||
		!(limited.Ledger().advancedTimestepS < limited.Ledger().requestedTimestepS) ||
		limited.Ledger().nonFiniteCells || !(limited.MinimumDensity() > 0.0) ||
		!(limited.MinimumPressure() > 0.0))
	{
		return Fail("runtime CFL-overload policy did not slow physical time safely");
	}
	const double limitedFirstAdvanced = limited.Ledger().advancedTimestepS;
	limited.Step();
	if (!limited.Ledger().acousticRoute || !limited.Ledger().timestepLimited ||
		!(limited.Ledger().advancedTimestepS < limited.Ledger().requestedTimestepS) ||
		limited.Ledger().nonFiniteCells || std::abs(limited.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(limited.Ledger().energyResidualJ()) > 1.0e-8)
	{
		return Fail("compressible event did not remain on the acoustic route after its source frame");
	}
	const double limitedSecondAdvanced = limited.Ledger().advancedTimestepS;

	auto topologyConfig = Config(24, 12, OmniAtmosphereBoundary::Periodic);
	topologyConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	OmniAtmosphere boundaryChange(topologyConfig);
	boundaryChange.ResetUniform(
		boundaryChange.Config().referenceDensity,
		boundaryChange.Config().referenceTemperature,
		0.05,
		0.0);
	boundaryChange.Step();
	if (boundaryChange.Ledger().acousticRoute || boundaryChange.Ledger().timestepLimited)
		return Fail("quiet periodic low-speed flow unexpectedly used the acoustic route");
	boundaryChange.SetBoundaryMode(OmniAtmosphereBoundary::Sealed);
	boundaryChange.Step();
	if (!boundaryChange.Ledger().acousticRoute || !boundaryChange.Ledger().timestepLimited ||
		std::abs(boundaryChange.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(boundaryChange.Ledger().momentumYResidual()) > 1.0e-12)
	{
		return Fail("reflecting boundary topology change did not activate a closed acoustic step");
	}

	OmniAtmosphere insertedWall(topologyConfig);
	insertedWall.ResetUniform(
		insertedWall.Config().referenceDensity,
		insertedWall.Config().referenceTemperature,
		0.05,
		0.0);
	insertedWall.Step();
	insertedWall.SetBlocked(12, 6, true);
	insertedWall.Step();
	if (!insertedWall.Ledger().acousticRoute || !insertedWall.Ledger().timestepLimited ||
		std::abs(insertedWall.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(insertedWall.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(insertedWall.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(insertedWall.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("inserted wall did not activate a conservative acoustic step");
	}

	auto cavityConfig = Config(3, 3, OmniAtmosphereBoundary::Sealed);
	cavityConfig.execution = OmniAtmosphereExecution::RuntimeLowMach;
	cavityConfig.maximumRuntimeSubsteps = 1;
	OmniAtmosphere cavity(cavityConfig);
	cavity.ResetUniform(
		cavity.Config().referenceDensity,
		cavity.Config().referenceTemperature,
		10.0,
		0.0);
	for (std::size_t y = 0; y < cavity.Height(); ++y)
	{
		for (std::size_t x = 0; x < cavity.Width(); ++x)
		{
			if (x != 1 || y != 1)
				cavity.SetBlocked(x, y, true);
		}
	}
	cavity.Step();
	cavity.Step();
	if (!cavity.Ledger().acousticRoute || !cavity.Ledger().timestepLimited ||
		std::abs(cavity.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(cavity.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(cavity.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(cavity.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("wall-normal cavity flow left the acoustic route before reflection resolved");
	}

	auto periodicSeamConfig = Config(3, 1, OmniAtmosphereBoundary::Periodic);
	periodicSeamConfig.execution = OmniAtmosphereExecution::ReferenceCompressible;
	OmniAtmosphere periodicSeam(periodicSeamConfig);
	periodicSeam.ResetUniform(
		periodicSeam.Config().referenceDensity,
		periodicSeam.Config().referenceTemperature);
	periodicSeam.SetBlocked(0, 0, true);
	periodicSeam.StepReference(1.0 / 60.0);
	if (std::abs(periodicSeam.State(1, 0).momentumX) > 1.0e-12 ||
		std::abs(periodicSeam.State(2, 0).momentumX) > 1.0e-12 ||
		std::abs(periodicSeam.Ledger().momentumXResidual()) > 1.0e-12 ||
		std::abs(periodicSeam.Ledger().momentumYResidual()) > 1.0e-12 ||
		std::abs(periodicSeam.Ledger().massResidualKg()) > 1.0e-14 ||
		std::abs(periodicSeam.Ledger().energyResidualJ()) > 1.0e-10)
	{
		return Fail("periodic blocked seam created a phantom impulse or ledger drift");
	}

	std::cout << "omni_atmosphere_cpu_mvp_pass=true\n";
	std::cout << "authoritative_gas_fields=rho_N2,rho_O2,rho_Ar,rho_CO2,rho_H2O,rho_u,rho_v,rho_E\n";
	std::cout << "eos=ideal_gas_mixture\n";
	std::cout << "uniform_mass_residual_kg=" << uniform.Ledger().massResidualKg() << '\n';
	std::cout << "uniform_energy_residual_j=" << uniform.Ledger().energyResidualJ() << '\n';
	std::cout << "heated_centre_pressure_pa=" << centrePressureAfterSource << '\n';
	std::cout << "near_vacuum_min_density=" << vacuum.MinimumDensity() << '\n';
	std::cout << "near_vacuum_min_pressure_pa=" << vacuum.MinimumPressure() << '\n';
	std::cout << "near_vacuum_pressure_floor_hits=" << vacuum.Ledger().pressureFloorHits << '\n';
	std::cout << "near_vacuum_energy_correction_j=" << vacuum.Ledger().numericalEnergyCorrectionJ << '\n';
	std::cout << "near_vacuum_energy_residual_j=" << vacuum.Ledger().energyResidualJ() << '\n';
	std::cout << "pressure_floor_available_thermal_energy_j=" << pressureFloorAvailable << '\n';
	std::cout << "internal_energy_floor_available_thermal_energy_j=" << internalFloorAvailable << '\n';
	std::cout << "positive_energy_overflow_rejected=true\n";
	std::cout << "rejected_reaction_result_cleared=true\n";
	std::cout << "serialized_latent_deficit_rejected=true\n";
	std::cout << "invalid_transport_coefficients_rejected=true\n";
	std::cout << "moving_water_phase_equilibrium=true\n";
	std::cout << "post_floor_phase_equilibrium=true\n";
	std::cout << "periodic_seam_only_diffusion=true\n";
	std::cout << "transport_stability_timestep_limited=true\n";
	std::cout << "transport_stability_advanced_timestep_s="
		<< transportLimited.Ledger().advancedTimestepS << '\n';
	std::cout << "water_diffusion_condensation_serializable=true\n";
	std::cout << "post_coupling_serializability_repair=true\n";
	std::cout << "gross_deficit_repair_rejected=true\n";
	std::cout << "nonfinite_deficit_repair_rejected=true\n";
	std::cout << "live_density_health_check=true\n";
	std::cout << "water_diffusion_energy_floor_hits="
		<< waterDiffusion.Ledger().energyFloorHits << '\n';
	std::cout << "water_diffusion_numerical_energy_correction_j="
		<< waterDiffusion.Ledger().numericalEnergyCorrectionJ << '\n';
	std::cout << "one_kelvin_diffusion_energy_floor_hits="
		<< floorBoundaryDiffusion.Ledger().energyFloorHits << '\n';
	std::cout << "leak_mass_before_kg=" << leakMassBefore << '\n';
	std::cout << "leak_mass_after_kg=" << leakMassAfter << '\n';
	std::cout << "runtime_grid_cells=" << runtime.CellCount() << '\n';
	std::cout << "authoritative_gas_bytes_per_cell="
		<< (OMNI_COMMON_SPECIES_COUNT + 3) * sizeof(double) << '\n';
	std::cout << "derived_density_cache_bytes_per_cell=" << sizeof(double) << '\n';
	std::cout << "condensed_water_sidecar_bytes_per_cell=" << sizeof(double) << '\n';
	std::cout << "working_state_bytes_per_cell="
		<< (sizeof(OmniAtmosphereConservative) * 2 +
			OMNI_COMMON_SPECIES_COUNT * sizeof(double) * 2 +
			sizeof(double) * 2 + sizeof(uint8_t)) << '\n';
	std::cout << "runtime_quiet_substeps=" << runtime.Ledger().substeps << '\n';
	std::cout << "runtime_quiet_ms_per_tick=" << runtimeMillisecondsPerTick << '\n';
	std::cout << "limited_requested_timestep_s=" << limited.Ledger().requestedTimestepS << '\n';
	std::cout << "limited_first_advanced_timestep_s=" << limitedFirstAdvanced << '\n';
	std::cout << "limited_second_advanced_timestep_s=" << limitedSecondAdvanced << '\n';
	std::cout << "limited_second_acoustic_route=" << (limited.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "wall_momentum_x_residual=" << wall.Ledger().momentumXResidual() << '\n';
	std::cout << "wall_momentum_y_residual=" << wall.Ledger().momentumYResidual() << '\n';
	std::cout << "blocked_edge_momentum_x_residual=" << blockedEdge.Ledger().momentumXResidual() << '\n';
	std::cout << "boundary_change_acoustic_route=" << (boundaryChange.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "inserted_wall_acoustic_route=" << (insertedWall.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "cavity_second_acoustic_route=" << (cavity.Ledger().acousticRoute ? "true" : "false") << '\n';
	std::cout << "periodic_blocked_seam_momentum_x_residual=" << periodicSeam.Ledger().momentumXResidual() << '\n';
	return 0;
}
