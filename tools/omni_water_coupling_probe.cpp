#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/ElementClasses.h"
#include "simulation/ElementDefs.h"
#include "simulation/OmniAtmosphere.h"
#include "simulation/OmniPhysicalScale.h"
#include "simulation/OmniThermal.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>

namespace
{
class ReplacementProbeSimulation : public Simulation
{
public:
	using Simulation::BeginOmniWaterCouplingTick;
	using Simulation::FinishOmniWaterCouplingTick;
	void UpdateParticles(int, int) override {}
};

class UpdateLifecycleProbeSimulation : public Simulation
{
public:
	int createdWater = -1;
	void UpdateParticles(int, int) override
	{
		if (createdWater < 0)
			createdWater = create_part(-1, 320, 200, PT_WATR);
	}
};

class UpdateDeleteProbeSimulation : public Simulation
{
public:
	int waterToDelete = -1;
	void UpdateParticles(int, int) override
	{
		if (waterToDelete >= 0)
		{
			kill_part(waterToDelete);
			waterToDelete = -1;
		}
	}
};

int Fail(std::string_view message)
{
	std::cerr << "omni-water-coupling-probe: FAIL " << message << std::endl;
	return 1;
}

void AdvanceOneTick(Simulation &simulation)
{
	simulation.BeforeSim(true);
	simulation.UpdateParticles(0, simulation.parts.active);
	simulation.AfterSim();
}

std::unique_ptr<Simulation> EnhancedSimulation()
{
	auto simulation = Simulation::Factory();
	simulation->gravityMode = GRAV_OFF;
	simulation->SetEdgeMode(EDGE_SOLID);
	simulation->SetOmniSimulationMode(OMNI_ENHANCED);
	return simulation;
}

double RunPressureEvaporation(float legacyPressure, double &resolvedPressurePa)
{
	auto simulation = EnhancedSimulation();
	constexpr int x = 160;
	constexpr int y = 160;
	const int cellX = x / CELL;
	const int cellY = y / CELL;
	for (int airY = 0; airY < YCELLS; ++airY)
	{
		for (int airX = 0; airX < XCELLS; ++airX)
		{
			simulation->pv[airY][airX] = legacyPressure;
			simulation->hv[airY][airX] = 350.0f;
		}
	}
	const int water = simulation->create_part(-1, x, y, PT_WATR);
	if (water < 0)
		return -1.0;
	simulation->parts[water].temp = 350.0f;
	const double before = simulation->GetOmniWaterParcelMassKg(water);
	simulation->BeforeSim(true);
	resolvedPressurePa = simulation->omniAtmosphere->Primitive(cellX, cellY).pressure;
	simulation->UpdateParticles(0, simulation->parts.active);
	simulation->AfterSim();
	return before - simulation->GetOmniWaterParcelMassKg(water);
}
}

int main()
{
	GlobalPrefs globalPrefs;
	SimulationData simulationData;

	auto injection = EnhancedSimulation();
	constexpr int vaporX = 120;
	constexpr int vaporY = 120;
	const int vapor = injection->create_part(-1, vaporX, vaporY, PT_WTRV);
	if (vapor < 0)
		return Fail("could not create an Enhanced WTRV injection parcel");
	const double parcelMassKg = injection->GetOmniWaterParcelMassKg(vapor);
	const double atmosphereWaterBefore = injection->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O);
	AdvanceOneTick(*injection);
	const auto injectionMetrics = injection->GetOmniWaterCouplingMetrics();
	if (injection->parts[vapor].type != PT_WTRV || !(injection->GetOmniWaterParcelMassKg(vapor) < parcelMassKg) ||
		!(injection->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) > atmosphereWaterBefore) ||
		std::abs((injection->GetOmniWaterParcelMassKg(vapor) + injectionMetrics.transferredToAtmosphereKg) - parcelMassKg) > 1.0e-14 ||
		injectionMetrics.vaporParcelsInjected != 1 ||
		std::abs(injectionMetrics.waterMassResidualKg) > 1.0e-12 ||
		std::abs(injectionMetrics.coupledEnergyResidualJ) > 1.0e-6)
	{
		std::cerr << "injection_debug parcel_mass=" << parcelMassKg
			<< " transferred=" << injectionMetrics.transferredToAtmosphereKg
			<< " mass_residual=" << injectionMetrics.waterMassResidualKg
			<< " energy_residual=" << injectionMetrics.coupledEnergyResidualJ << '\n';
		return Fail("WTRV injection did not conserve water mass and coupled energy");
	}

	auto evaporation = EnhancedSimulation();
	constexpr int waterX = 200;
	constexpr int waterY = 180;
	const int water = evaporation->create_part(-1, waterX, waterY, PT_WATR);
	if (water < 0)
		return Fail("could not create an Enhanced liquid-water parcel");
	evaporation->parts[water].temp = 500.0f;
	const double waterMassBefore = evaporation->GetOmniWaterParcelMassKg(water);
	const double gasWaterBefore = evaporation->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O);
	AdvanceOneTick(*evaporation);
	const auto evaporationMetrics = evaporation->GetOmniWaterCouplingMetrics();
	const double waterMassAfter = evaporation->GetOmniWaterParcelMassKg(water);
	const double waterEnthalpyAfter =
		evaporation->GetOmniWaterParcelSpecificEnthalpyJPerKg(water);
	if (!(waterMassAfter < waterMassBefore) ||
		!(evaporation->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) > gasWaterBefore) ||
		evaporationMetrics.evaporationTransfers != 1 ||
		std::abs(evaporationMetrics.waterMassResidualKg) > 1.0e-12 ||
		std::abs(evaporationMetrics.coupledEnergyResidualJ) > 1.0e-6)
	{
		std::cerr << "evaporation_debug before=" << waterMassBefore
			<< " after=" << waterMassAfter
			<< " mass_residual=" << evaporationMetrics.waterMassResidualKg
			<< " energy_residual=" << evaporationMetrics.coupledEnergyResidualJ
			<< " sensible_to_particle=" << evaporationMetrics.sensibleEnergyToParticlesJ
			<< " particle_removed=" << evaporationMetrics.particleEnergyRemovedJ
			<< " atmosphere_added=" << evaporationMetrics.atmosphereEnergyAddedJ << '\n';
		return Fail("liquid-water evaporation did not conserve mass and energy");
	}

	double lowResolvedPressurePa = 0.0;
	double highResolvedPressurePa = 0.0;
	const double lowPressureEvaporationKg = RunPressureEvaporation(-80.0f, lowResolvedPressurePa);
	const double highPressureEvaporationKg = RunPressureEvaporation(120.0f, highResolvedPressurePa);
	if (!(lowPressureEvaporationKg > highPressureEvaporationKg) || !(highPressureEvaporationKg >= 0.0))
	{
		std::cerr << "boiling_debug low_pressure_kg=" << lowPressureEvaporationKg
			<< " high_pressure_kg=" << highPressureEvaporationKg
			<< " low_pressure_pa=" << lowResolvedPressurePa
			<< " high_pressure_pa=" << highResolvedPressurePa
			<< " particle_saturation_pa=" << OmniThermal::SaturationPressurePa(350.0) << '\n';
		return Fail("pressure-dependent boiling tendency was not stronger at low pressure");
	}

	auto phase = EnhancedSimulation();
	const int freezing = phase->create_part(-1, 260, 180, PT_WATR);
	const int melting = phase->create_part(-1, 280, 180, PT_ICEI);
	if (freezing < 0 || melting < 0)
		return Fail("could not create water phase fixtures");
	phase->parts[freezing].temp = 250.0f;
	phase->parts[melting].temp = 310.0f;
	AdvanceOneTick(*phase);
	if (phase->parts[freezing].type != PT_ICEI || phase->parts[melting].type != PT_WATR ||
		phase->GetOmniWaterCouplingMetrics().phaseTypeChanges != 2)
	{
		return Fail("managed water enthalpy projection did not freeze and melt correctly");
	}

	// WATR is conductive, and the legacy fast spark path temporarily wraps it
	// as SPRK(WATR). The Omni parcel must remain attached to the underlying
	// water through an OPS checkpoint and the later SPRK -> WATR restoration.
	auto spark = EnhancedSimulation();
	constexpr int sparkX = 300;
	constexpr int sparkY = 200;
	const int sparkWater = spark->create_part(-1, sparkX, sparkY, PT_WATR);
	if (sparkWater < 0)
		return Fail("could not create the sparked-water persistence fixture");
	spark->parts[sparkWater].temp = 330.0f;
	AdvanceOneTick(*spark);
	if (spark->parts[sparkWater].type != PT_WATR)
		return Fail("heated sparked-water fixture changed phase before the spark test");
	const double sparkMassBefore = spark->GetOmniWaterParcelMassKg(sparkWater);
	const double sparkEnthalpyBefore = spark->GetOmniWaterParcelSpecificEnthalpyJPerKg(sparkWater);
	const int sparked = spark->create_part(-1, sparkX, sparkY, PT_SPRK);
	if (sparked != sparkWater || spark->parts[sparkWater].type != PT_SPRK ||
		spark->parts[sparkWater].ctype != PT_WATR ||
		spark->GetOmniWaterParcelMassKg(sparkWater) != sparkMassBefore ||
		spark->GetOmniWaterParcelSpecificEnthalpyJPerKg(sparkWater) != sparkEnthalpyBefore)
	{
		return Fail("SPRK fast path detached the underlying WATR parcel state");
	}
	auto sparkSave = spark->Save(true, RES.OriginRect());
	if (!sparkSave)
		return Fail("could not create the sparked-water OPS checkpoint");
	const auto sparkSerialised = sparkSave->Serialise();
	if (sparkSerialised.first || sparkSerialised.second.empty())
		return Fail("SPRK(WATR) parcel state was rejected by OPS serialization");
	GameSave parsedSpark(sparkSerialised.second);
	auto restoredSpark = Simulation::Factory();
	restoredSpark->SetOmniSimulationMode(parsedSpark.omniSimulationMode);
	restoredSpark->Load(&parsedSpark, true, { 0, 0 });
	int restoredSparkWater = -1;
	for (int index = 0; index < restoredSpark->parts.active; ++index)
	{
		if (restoredSpark->parts[index].type == PT_SPRK &&
			restoredSpark->parts[index].ctype == PT_WATR)
		{
			restoredSparkWater = index;
			break;
		}
	}
	if (restoredSparkWater < 0 ||
		restoredSpark->GetOmniWaterParcelMassKg(restoredSparkWater) != sparkMassBefore ||
		restoredSpark->GetOmniWaterParcelSpecificEnthalpyJPerKg(restoredSparkWater) !=
			sparkEnthalpyBefore)
	{
		return Fail("SPRK(WATR) parcel state did not survive the OPS round trip");
	}

	// Exercise an existing-ID transition sequence.  The initial PT_SPRK call
	// deliberately uses the legacy spark fast path; the following ICEI/WATR
	// calls use the general replacement path.  This proves sidecar continuity
	// across both paths, but does not claim that the generic t == PT_SPRK branch
	// is reachable through this public call shape.
	auto directReplacement = std::make_unique<ReplacementProbeSimulation>();
	directReplacement->SetOmniSimulationMode(OMNI_ENHANCED);
	constexpr int replaceX = 340;
	constexpr int replaceY = 200;
	const int replaceWater = directReplacement->create_part(-1, replaceX, replaceY, PT_WATR);
	if (replaceWater < 0)
		return Fail("could not create direct replacement fixture");
	const double replaceMass = directReplacement->GetOmniWaterParcelMassKg(replaceWater);
	const double replaceEnthalpy =
		directReplacement->GetOmniWaterParcelSpecificEnthalpyJPerKg(replaceWater);
	if (directReplacement->create_part(replaceWater, replaceX, replaceY, PT_SPRK) != replaceWater ||
		directReplacement->parts[replaceWater].type != PT_SPRK ||
		directReplacement->parts[replaceWater].ctype != PT_WATR)
	{
		return Fail("create_part existing-ID managed replacement detached WATR state");
	}
	if (directReplacement->create_part(replaceWater, replaceX, replaceY, PT_ICEI) != replaceWater ||
		directReplacement->GetOmniWaterParcelMassKg(replaceWater) != replaceMass ||
		directReplacement->GetOmniWaterParcelSpecificEnthalpyJPerKg(replaceWater) != replaceEnthalpy)
		return Fail("create_part existing-ID WATR -> ICEI replacement detached WATR state");
	const auto iceThermalState = OmniThermal::WaterFromSpecificEnthalpy(replaceEnthalpy);
	if (!std::isfinite(iceThermalState.temperatureK) ||
		std::abs(double(directReplacement->parts[replaceWater].temp) -
			iceThermalState.temperatureK) > 1.0e-3)
		return Fail("WATR -> ICEI replacement left public temperature inconsistent with enthalpy");
	if (directReplacement->create_part(replaceWater, replaceX, replaceY, PT_WATR) != replaceWater ||
		directReplacement->GetOmniWaterParcelMassKg(replaceWater) != replaceMass ||
		directReplacement->GetOmniWaterParcelSpecificEnthalpyJPerKg(replaceWater) != replaceEnthalpy)
		return Fail("create_part existing-ID ICEI -> WATR replacement detached WATR state");
	directReplacement->BeginOmniWaterCouplingTick();
	if (directReplacement->create_part(replaceWater, replaceX, replaceY, PT_DUST) != replaceWater)
		return Fail("create_part existing-ID non-water replacement failed");
	directReplacement->FinishOmniWaterCouplingTick();
	const auto replacementMetrics = directReplacement->GetOmniWaterCouplingMetrics();
	if (directReplacement->GetOmniWaterParcelMassKg(replaceWater) != 0.0 ||
		directReplacement->GetOmniWaterParcelSpecificEnthalpyJPerKg(replaceWater) != 0.0 ||
		std::abs(replacementMetrics.externalWaterMassSinkKg - replaceMass) > 1.0e-15 ||
		std::abs(replacementMetrics.externalWaterEnergySinkJ - replaceMass * replaceEnthalpy) > 1.0e-9 ||
		std::abs(replacementMetrics.waterMassResidualKg) > 1.0e-12 ||
		std::abs(replacementMetrics.coupledEnergyResidualJ) > 1.0e-6)
	{
		return Fail("non-water replacement did not clear and reconcile the WATR sidecar sink");
	}
	restoredSpark->parts[restoredSparkWater].life = 0;
	AdvanceOneTick(*restoredSpark);
	const auto restoredThermalState = OmniThermal::WaterFromSpecificEnthalpy(sparkEnthalpyBefore);
	if (restoredSpark->parts[restoredSparkWater].type != PT_WATR ||
		restoredSpark->GetOmniWaterParcelMassKg(restoredSparkWater) != sparkMassBefore ||
		restoredSpark->GetOmniWaterParcelSpecificEnthalpyJPerKg(restoredSparkWater) !=
			sparkEnthalpyBefore ||
		!std::isfinite(restoredThermalState.temperatureK) ||
		std::abs(double(restoredSpark->parts[restoredSparkWater].temp) -
			restoredThermalState.temperatureK) > 1.0e-3 ||
		std::abs(restoredSpark->GetOmniWaterCouplingMetrics().waterMassResidualKg) > 1.0e-12 ||
		std::abs(restoredSpark->GetOmniWaterCouplingMetrics().coupledEnergyResidualJ) > 1.0e-6)
	{
		return Fail("SPRK -> WATR restoration did not preserve parcel state, temperature, and balance");
	}
	AdvanceOneTick(*restoredSpark);
	const auto postDesparkMetrics = restoredSpark->GetOmniWaterCouplingMetrics();
	if (!std::isfinite(postDesparkMetrics.coupledEnergyResidualJ) ||
		std::abs(postDesparkMetrics.coupledEnergyResidualJ) > 1.0e-6 ||
		std::abs(postDesparkMetrics.waterMassResidualKg) > 1.0e-12)
		return Fail("post-despark coupling imported unledgered water energy");

	auto updateLifecycle = std::make_unique<UpdateLifecycleProbeSimulation>();
	updateLifecycle->gravityMode = GRAV_OFF;
	updateLifecycle->SetEdgeMode(EDGE_SOLID);
	updateLifecycle->SetOmniSimulationMode(OMNI_ENHANCED);
	AdvanceOneTick(*updateLifecycle);
	const auto updateLifecycleMetrics = updateLifecycle->GetOmniWaterCouplingMetrics();
	const double updateLifecycleMass = updateLifecycle->GetOmniWaterParcelMassKg(
		updateLifecycle->createdWater);
	if (updateLifecycle->createdWater < 0 || !(updateLifecycleMass > 0.0) ||
		std::abs(updateLifecycleMetrics.externalWaterMassSourceKg - updateLifecycleMass) > 1.0e-15 ||
		std::abs(updateLifecycleMetrics.waterMassResidualKg) > 1.0e-12 ||
		std::abs(updateLifecycleMetrics.coupledEnergyResidualJ) > 1.0e-6)
	{
		std::cerr << "update_lifecycle_debug source=" << updateLifecycleMetrics.externalWaterMassSourceKg
			<< " mass=" << updateLifecycleMass
			<< " residual=" << updateLifecycleMetrics.waterMassResidualKg
			<< " energy_residual=" << updateLifecycleMetrics.coupledEnergyResidualJ << '\n';
		return Fail("water created during UpdateParticles was double-counted by AfterSim rebase");
	}

	auto updateDelete = std::make_unique<UpdateDeleteProbeSimulation>();
	updateDelete->gravityMode = GRAV_OFF;
	updateDelete->SetEdgeMode(EDGE_SOLID);
	updateDelete->SetOmniSimulationMode(OMNI_ENHANCED);
	updateDelete->waterToDelete = updateDelete->create_part(-1, 340, 200, PT_WATR);
	if (updateDelete->waterToDelete < 0)
		return Fail("could not create UpdateParticles deletion fixture");
	const double updateDeleteMass = updateDelete->GetOmniWaterParcelMassKg(
		updateDelete->waterToDelete);
	const double updateDeleteEnergy = updateDeleteMass *
		updateDelete->GetOmniWaterParcelSpecificEnthalpyJPerKg(updateDelete->waterToDelete);
	AdvanceOneTick(*updateDelete);
	const auto updateDeleteMetrics = updateDelete->GetOmniWaterCouplingMetrics();
	if (std::abs(updateDeleteMetrics.externalWaterMassSinkKg - updateDeleteMass) > 1.0e-15 ||
		std::abs(updateDeleteMetrics.externalWaterEnergySinkJ - updateDeleteEnergy) > 1.0e-9 ||
		std::abs(updateDeleteMetrics.waterMassResidualKg) > 1.0e-12 ||
		std::abs(updateDeleteMetrics.coupledEnergyResidualJ) > 1.0e-6)
	{
		return Fail("water deleted during UpdateParticles was double-counted by AfterSim rebase");
	}

	auto save = evaporation->Save(true, RES.OriginRect());
	if (!save || !save->hasOmniWaterParcelState ||
		save->omniWaterParcelStateVersion != GameSave::OmniWaterParcelStateVersion)
	{
		return Fail("Enhanced save omitted water parcel sidecar state");
	}
	auto serialised = save->Serialise().second;
	if (serialised.empty())
		return Fail("Enhanced water parcel OPS serialization failed");
	GameSave parsed(serialised);
	if (!parsed.hasOmniWaterParcelState ||
		parsed.omniWaterParcelStateVersion != GameSave::OmniWaterParcelStateVersion ||
		parsed.omniWaterParcelMassKg.size() != static_cast<size_t>(parsed.particlesCount) ||
		parsed.omniWaterParcelSpecificEnthalpyJPerKg.size() !=
			static_cast<size_t>(parsed.particlesCount))
	{
		return Fail("Enhanced water parcel OPS parsing failed");
	}
	auto restored = Simulation::Factory();
	restored->SetOmniSimulationMode(parsed.omniSimulationMode);
	restored->Load(&parsed, true, { 0, 0 });
	int restoredWater = -1;
	for (int index = 0; index < restored->parts.active; ++index)
	{
		if (restored->parts[index].type == PT_WATR || restored->parts[index].type == PT_ICEI)
		{
			restoredWater = index;
			break;
		}
	}
	if (restoredWater < 0 ||
		std::abs(restored->GetOmniWaterParcelMassKg(restoredWater) - waterMassAfter) > 1.0e-15 ||
		std::abs(restored->GetOmniWaterParcelSpecificEnthalpyJPerKg(restoredWater) -
			waterEnthalpyAfter) > 1.0e-9)
	{
		std::cerr << "sidecar_debug expected=" << waterMassAfter
			<< " restored=" << (restoredWater < 0 ? -1.0 : restored->GetOmniWaterParcelMassKg(restoredWater))
			<< " parsed_count=" << parsed.omniWaterParcelMassKg.size()
			<< " particles=" << parsed.particlesCount
			<< " parsed_type=" << (parsed.particlesCount > 0 ? parsed.particles[0].type : -1)
			<< " restored_active=" << restored->parts.active << '\n';
		return Fail("water particle mass/enthalpy sidecars did not survive OPS v2 round trip");
	}

	// Version 1 remains readable. Its mass-only payload derives the missing
	// enthalpy from the saved public temperature during parse/load.
	GameSave legacyWater = *save;
	legacyWater.omniWaterParcelStateVersion = GameSave::OmniWaterParcelLegacyStateVersion;
	legacyWater.omniWaterParcelSpecificEnthalpyJPerKg.clear();
	const auto legacySerialised = legacyWater.Serialise().second;
	if (legacySerialised.empty())
		return Fail("water parcel OPS v1 compatibility fixture could not be serialized");
	GameSave parsedLegacyWater(legacySerialised);
	if (!parsedLegacyWater.hasOmniWaterParcelState ||
		parsedLegacyWater.omniWaterParcelStateVersion != GameSave::OmniWaterParcelLegacyStateVersion ||
		parsedLegacyWater.omniWaterParcelSpecificEnthalpyJPerKg.size() !=
			static_cast<size_t>(parsedLegacyWater.particlesCount))
	{
		return Fail("water parcel OPS v1 did not migrate to an enthalpy-bearing in-memory state");
	}
	for (int index = 0; index < parsedLegacyWater.particlesCount; ++index)
	{
		const double expected = OmniThermal::WaterSpecificEnthalpyJPerKg(
			parsedLegacyWater.particles[index].temp);
		if (std::abs(parsedLegacyWater.omniWaterParcelSpecificEnthalpyJPerKg[index] - expected) >
			1.0e-9)
		{
			return Fail("water parcel OPS v1 migration did not derive enthalpy from temperature");
		}
	}

	auto drift = EnhancedSimulation();
	const int driftWater = drift->create_part(-1, 320, 180, PT_WATR);
	if (driftWater < 0)
		return Fail("could not create the long-run water fixture");
	drift->parts[driftWater].temp = 450.0f;
	const double driftInitialWaterKg = drift->TotalOmniParticleWaterMassKg() +
		drift->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) +
		drift->omniAtmosphere->TotalCondensedWaterMassKg();
	double maximumStepWaterResidualKg = 0.0;
	double maximumStepEnergyResidualJ = 0.0;
	double maximumAtmosphereSpeciesResidualKg = 0.0;
	double accumulatedNumericalWaterCorrectionKg = 0.0;
	double accumulatedBoundaryWaterOutKg = 0.0;
	double accumulatedBoundaryWaterInKg = 0.0;
	double accumulatedSourceWaterKg = 0.0;
	for (int step = 0; step < 1000; ++step)
	{
		drift->BeforeSim(true);
		drift->UpdateParticles(0, drift->parts.active);
		drift->AfterSim();
		const auto metrics = drift->GetOmniWaterCouplingMetrics();
		maximumStepWaterResidualKg = std::max(maximumStepWaterResidualKg, std::abs(metrics.waterMassResidualKg));
		maximumStepEnergyResidualJ = std::max(maximumStepEnergyResidualJ, std::abs(metrics.coupledEnergyResidualJ));
		const auto &atmosphereLedger = drift->omniAtmosphere->Ledger();
		maximumAtmosphereSpeciesResidualKg = std::max(maximumAtmosphereSpeciesResidualKg,
			std::abs(atmosphereLedger.speciesMassResidualKg(OMNI_SPECIES_H2O)));
		accumulatedNumericalWaterCorrectionKg += atmosphereLedger.numericalSpeciesCorrectionKg[OMNI_SPECIES_H2O];
		accumulatedBoundaryWaterOutKg += atmosphereLedger.boundarySpeciesOutKg[OMNI_SPECIES_H2O];
		accumulatedBoundaryWaterInKg += atmosphereLedger.boundarySpeciesInKg[OMNI_SPECIES_H2O];
		accumulatedSourceWaterKg += atmosphereLedger.sourceSpeciesMassKg[OMNI_SPECIES_H2O];
	}
	const double driftFinalWaterKg = drift->TotalOmniParticleWaterMassKg() +
		drift->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O) +
		drift->omniAtmosphere->TotalCondensedWaterMassKg();
	if (std::abs(driftFinalWaterKg - driftInitialWaterKg) > 1.0e-10 ||
		maximumStepWaterResidualKg > 1.0e-10 || maximumStepEnergyResidualJ > 1.0e-6)
	{
		std::cerr << "drift_debug initial_water=" << driftInitialWaterKg
			<< " final_water=" << driftFinalWaterKg
			<< " particle_water=" << drift->TotalOmniParticleWaterMassKg()
			<< " gas_water=" << drift->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_H2O)
			<< " condensed_water=" << drift->omniAtmosphere->TotalCondensedWaterMassKg()
			<< " max_mass_residual=" << maximumStepWaterResidualKg
			<< " max_energy_residual=" << maximumStepEnergyResidualJ
			<< " atmosphere_species_residual=" << maximumAtmosphereSpeciesResidualKg
			<< " numerical_water_correction=" << accumulatedNumericalWaterCorrectionKg
			<< " boundary_water_out=" << accumulatedBoundaryWaterOutKg
			<< " boundary_water_in=" << accumulatedBoundaryWaterInKg
			<< " source_water=" << accumulatedSourceWaterKg << '\n';
		return Fail("1000-step water/energy drift exceeded the runtime tolerance");
	}

	std::cout << "omni_water_coupling_probe_pass=true\n";
	std::cout << "default_water_parcel_mass_kg=" << OmniPhysicalScale::DefaultWaterParcelMassKg << '\n';
	std::cout << "wtrv_injected_mass_kg=" << injectionMetrics.transferredToAtmosphereKg << '\n';
	std::cout << "evaporated_mass_kg=" << waterMassBefore - waterMassAfter << '\n';
	std::cout << "low_pressure_evaporation_kg=" << lowPressureEvaporationKg << '\n';
	std::cout << "high_pressure_evaporation_kg=" << highPressureEvaporationKg << '\n';
	std::cout << "water_mass_residual_kg=" << evaporationMetrics.waterMassResidualKg << '\n';
	std::cout << "coupled_energy_residual_j=" << evaporationMetrics.coupledEnergyResidualJ << '\n';
	std::cout << "water_sidecar_bytes_per_particle=" << 2 * sizeof(double) << '\n';
	std::cout << "water_sidecar_ops_roundtrip=true\n";
	std::cout << "water_sidecar_v1_migration=true\n";
	std::cout << "water_lifecycle_type_change_replacement_delete=true\n";
	std::cout << "water_existing_id_replacement_probe=true\n";
	std::cout << "update_phase_water_creation_ledger_closed=true\n";
	std::cout << "update_phase_water_deletion_ledger_closed=true\n";
	std::cout << "long_run_steps=1000\n";
	std::cout << "long_run_water_drift_kg=" << driftFinalWaterKg - driftInitialWaterKg << '\n';
	std::cout << "long_run_max_energy_residual_j=" << maximumStepEnergyResidualJ << '\n';
	return 0;
}
