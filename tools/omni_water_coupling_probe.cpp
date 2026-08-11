#include "client/GameSave.h"
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
		parsed.omniWaterParcelMassKg.size() != static_cast<size_t>(parsed.particlesCount))
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
		std::abs(restored->GetOmniWaterParcelMassKg(restoredWater) - waterMassAfter) > 1.0e-15)
	{
		std::cerr << "sidecar_debug expected=" << waterMassAfter
			<< " restored=" << (restoredWater < 0 ? -1.0 : restored->GetOmniWaterParcelMassKg(restoredWater))
			<< " parsed_count=" << parsed.omniWaterParcelMassKg.size()
			<< " particles=" << parsed.particlesCount
			<< " parsed_type=" << (parsed.particlesCount > 0 ? parsed.particles[0].type : -1)
			<< " restored_active=" << restored->parts.active << '\n';
		return Fail("water particle mass sidecar did not survive OPS round trip");
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
	std::cout << "water_sidecar_bytes_per_particle=" << sizeof(double) << '\n';
	std::cout << "water_sidecar_ops_roundtrip=true\n";
	std::cout << "long_run_steps=1000\n";
	std::cout << "long_run_water_drift_kg=" << driftFinalWaterKg - driftInitialWaterKg << '\n';
	std::cout << "long_run_max_energy_residual_j=" << maximumStepEnergyResidualJ << '\n';
	return 0;
}
