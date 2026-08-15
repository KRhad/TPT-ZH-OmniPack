#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/ElementClasses.h"
#include "simulation/ElementDefs.h"
#include "simulation/OmniAtmosphere.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace
{
class ProbeSimulation : public Simulation
{
public:
	using Simulation::BeginOmniChemistryTick;
	using Simulation::BeginOmniWaterCouplingTick;
	using Simulation::FinishOmniChemistryTick;
	using Simulation::FinishOmniWaterCouplingTick;
	void SetWaterParcelMassKg(int particleId, double massKg)
	{
		omniWaterParcelMassKg[particleId] = massKg;
	}
	void SetCarbonParcelMassKg(int particleId, double massKg)
	{
		omniCarbonParcelMassKg[particleId] = massKg;
	}
	void UpdateParticles(int, int) override {}
};

int Fail(std::string_view message)
{
	std::cerr << "omni-combustion-probe: FAIL " << message << '\n';
	return 1;
}

void AdvanceOneTick(Simulation &simulation)
{
	simulation.BeforeSim(true);
	simulation.UpdateParticles(0, simulation.parts.active);
	simulation.AfterSim();
}

std::unique_ptr<Simulation> MakeSimulation(int mode, double oxygenMassFraction)
{
	auto simulation = Simulation::Factory();
	simulation->gravityMode = GRAV_OFF;
	simulation->SetEdgeMode(EDGE_SOLID);
	simulation->SetOmniSimulationMode(mode);
	if (mode != OMNI_CLASSIC)
	{
		const double o2 = std::clamp(oxygenMassFraction, 0.0, 1.0);
		const std::vector<double> fractions{ 1.0 - o2, o2, 0.0, 0.0, 0.0 };
		for (int y = 0; y < YCELLS; ++y)
			for (int x = 0; x < XCELLS; ++x)
				simulation->omniAtmosphere->SetSpeciesMassFractions(x, y, fractions);
	}
	return simulation;
}

struct Sample
{
	double carbonBefore = 0.0;
	double carbonAfter = 0.0;
	double oxygenBefore = 0.0;
	double oxygenAfter = 0.0;
	double carbonDioxideBefore = 0.0;
	double carbonDioxideAfter = 0.0;
	Simulation::OmniChemistryMetrics metrics{};
	std::unique_ptr<Simulation> simulation;
	int coal = -1;
};

Sample Run(double oxygenMassFraction)
{
	Sample sample;
	sample.simulation = MakeSimulation(OMNI_ENHANCED, oxygenMassFraction);
	sample.coal = sample.simulation->create_part(-1, 240, 180, PT_COAL);
	if (sample.coal < 0)
		return sample;
	sample.simulation->parts[sample.coal].temp = 1200.0f;
	sample.carbonBefore = sample.simulation->GetOmniCarbonParcelMassKg(sample.coal);
	sample.oxygenBefore = sample.simulation->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_O2);
	sample.carbonDioxideBefore = sample.simulation->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_CO2);
	AdvanceOneTick(*sample.simulation);
	sample.carbonAfter = sample.simulation->GetOmniCarbonParcelMassKg(sample.coal);
	sample.oxygenAfter = sample.simulation->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_O2);
	sample.carbonDioxideAfter = sample.simulation->omniAtmosphere->TotalSpeciesMassKg(OMNI_SPECIES_CO2);
	sample.metrics = sample.simulation->GetOmniChemistryMetrics();
	return sample;
}
}

int main()
{
	GlobalPrefs globalPrefs;
	SimulationData simulationData;
	auto low = Run(0.02);
	auto normal = Run(0.231374517735);
	auto high = Run(0.90);
	if (low.coal < 0 || normal.coal < 0 || high.coal < 0)
		return Fail("could not create controlled COAL fixtures");
	for (const auto *sample : { &low, &normal, &high })
	{
		if (sample->metrics.committedTransactions != 1 ||
			!(sample->metrics.carbonConsumedKg > 0.0) ||
			!(sample->carbonAfter < sample->carbonBefore) ||
			!(sample->oxygenAfter < sample->oxygenBefore) ||
			!(sample->carbonDioxideAfter > sample->carbonDioxideBefore) ||
			std::abs(sample->metrics.massResidualKg) > 5.0e-12 ||
			std::abs(sample->metrics.carbonAtomResidualMol) > 5.0e-10 ||
			std::abs(sample->metrics.oxygenAtomResidualMol) > 5.0e-10 ||
			std::abs(sample->metrics.energyResidualJ) > 1.0e-9)
		{
			return Fail("Enhanced combustion transaction did not close its ledgers");
		}
	}
	if (!(low.metrics.oxygenConsumedKg < normal.metrics.oxygenConsumedKg &&
		normal.metrics.oxygenConsumedKg < high.metrics.oxygenConsumedKg))
	{
		return Fail("oxygen concentration did not control the reaction amount");
	}

	auto cold = MakeSimulation(OMNI_ENHANCED, 0.90);
	const int coldCoal = cold->create_part(-1, 240, 180, PT_COAL);
	cold->parts[coldCoal].temp = 293.15f;
	const double coldCarbonBefore = cold->GetOmniCarbonParcelMassKg(coldCoal);
	AdvanceOneTick(*cold);
	if (cold->GetOmniChemistryMetrics().committedTransactions != 0 ||
		cold->GetOmniCarbonParcelMassKg(coldCoal) != coldCarbonBefore)
	{
		return Fail("room-temperature carbon reacted without ignition energy");
	}
	low.simulation.reset();
	high.simulation.reset();
	cold.reset();

	auto classic = MakeSimulation(OMNI_CLASSIC, 0.90);
	const int classicCoal = classic->create_part(-1, 240, 180, PT_COAL);
	classic->parts[classicCoal].temp = 1200.0f;
	AdvanceOneTick(*classic);
	if (classic->GetOmniChemistryMetrics().committedTransactions != 0 ||
		classic->GetOmniCarbonParcelMassKg(classicCoal) != 0.0)
	{
		return Fail("Classic unexpectedly entered the OmniReactionRuntime path");
	}

	auto modeSwitch = std::make_unique<ProbeSimulation>();
	modeSwitch->gravityMode = GRAV_OFF;
	modeSwitch->SetEdgeMode(EDGE_SOLID);
	const int switchedWater = modeSwitch->create_part(-1, 280, 180, PT_WATR);
	const int switchedCoal = modeSwitch->create_part(-1, 300, 180, PT_COAL);
	if (switchedWater < 0 || switchedCoal < 0 ||
		modeSwitch->GetOmniWaterParcelMassKg(switchedWater) != 0.0 ||
		modeSwitch->GetOmniCarbonParcelMassKg(switchedCoal) != 0.0)
	{
		return Fail("Classic mode-switch fixtures unexpectedly owned Enhanced sidecars");
	}
	modeSwitch->SetOmniSimulationMode(OMNI_ENHANCED);
	const double switchedWaterMass = modeSwitch->GetOmniWaterParcelMassKg(switchedWater);
	const double switchedWaterEnthalpy =
		modeSwitch->GetOmniWaterParcelSpecificEnthalpyJPerKg(switchedWater);
	const double switchedCarbonMass = modeSwitch->GetOmniCarbonParcelMassKg(switchedCoal);
	if (!(switchedWaterMass > 0.0) || !std::isfinite(switchedWaterEnthalpy) ||
		!(switchedWaterEnthalpy > 0.0) || !(switchedCarbonMass > 0.0))
	{
		return Fail("Classic-to-Enhanced mode switch did not initialize water/carbon sidecars");
	}
	auto modeSwitchSave = modeSwitch->Save(true, RES.OriginRect());
	if (!modeSwitchSave || !modeSwitchSave->hasOmniWaterParcelState ||
		!modeSwitchSave->hasOmniCarbonParcelState)
	{
		return Fail("Classic-to-Enhanced pre-tick save omitted initialized sidecars");
	}
	AdvanceOneTick(*modeSwitch);
	const auto modeSwitchChemistry = modeSwitch->GetOmniChemistryMetrics();
	if (modeSwitchChemistry.committedTransactions != 0 ||
		std::abs(modeSwitchChemistry.massResidualKg) > 1.0e-15 ||
		std::abs(modeSwitchChemistry.totalMassBalanceResidualKg) > 1.0e-15)
	{
		return Fail("Classic-to-Enhanced first tick treated default carbon as an unexplained source");
	}
	const double preservedWaterMass = switchedWaterMass * 0.5;
	const double preservedCarbonMass = switchedCarbonMass * 0.5;
	modeSwitch->SetWaterParcelMassKg(switchedWater, preservedWaterMass);
	modeSwitch->SetCarbonParcelMassKg(switchedCoal, preservedCarbonMass);
	modeSwitch->SetOmniSimulationMode(OMNI_CLASSIC);
	modeSwitch->SetOmniSimulationMode(OMNI_ENHANCED);
	if (modeSwitch->GetOmniWaterParcelMassKg(switchedWater) != preservedWaterMass ||
		modeSwitch->GetOmniCarbonParcelMassKg(switchedCoal) != preservedCarbonMass)
	{
		return Fail("Enhanced sidecars were reset across a Classic round trip");
	}

	auto lifecycle = std::make_unique<ProbeSimulation>();
	lifecycle->SetOmniSimulationMode(OMNI_ENHANCED);
	const int changedCoal = lifecycle->create_part(-1, 240, 180, PT_COAL);
	const double changedCarbon = lifecycle->GetOmniCarbonParcelMassKg(changedCoal);
	lifecycle->BeginOmniChemistryTick();
	if (changedCoal < 0 || !(changedCarbon > 0.0) ||
		lifecycle->part_change_type(changedCoal, 240, 180, PT_DUST))
	{
		return Fail("could not create carbon type-change ledger fixture");
	}
	lifecycle->FinishOmniChemistryTick();
	const auto lifecycleMetrics = lifecycle->GetOmniChemistryMetrics();
	if (std::abs(lifecycleMetrics.externalCarbonMassSinkKg - changedCarbon) > 1.0e-15 ||
		std::abs(lifecycleMetrics.massResidualKg + changedCarbon) > 1.0e-15 ||
		std::abs(lifecycleMetrics.carbonAtomResidualMol +
			changedCarbon / 0.0120107) > 1.0e-12 ||
		std::abs(lifecycleMetrics.totalMassBalanceResidualKg) > 1.0e-15 ||
		std::abs(lifecycleMetrics.totalCarbonAtomBalanceResidualMol) > 1.0e-12)
	{
		return Fail("COAL type change bypassed the external carbon sink ledger");
	}

	auto replacement = std::make_unique<ProbeSimulation>();
	replacement->SetOmniSimulationMode(OMNI_ENHANCED);
	const int replacedCoal = replacement->create_part(-1, 240, 180, PT_COAL);
	const double replacedCarbon = replacement->GetOmniCarbonParcelMassKg(replacedCoal);
	replacement->BeginOmniChemistryTick();
	if (replacedCoal < 0 || replacement->create_part(replacedCoal, 240, 180, PT_WOOD) != replacedCoal)
		return Fail("could not create carbon replacement ledger fixture");
	replacement->FinishOmniChemistryTick();
	const auto replacementMetrics = replacement->GetOmniChemistryMetrics();
	if (std::abs(replacementMetrics.externalCarbonMassSinkKg - replacedCarbon) > 1.0e-15 ||
		std::abs(replacementMetrics.massResidualKg + replacedCarbon) > 1.0e-15 ||
		std::abs(replacementMetrics.carbonAtomResidualMol +
			replacedCarbon / 0.0120107) > 1.0e-12 ||
		std::abs(replacementMetrics.totalMassBalanceResidualKg) > 1.0e-15 ||
		std::abs(replacementMetrics.totalCarbonAtomBalanceResidualMol) > 1.0e-12)
	{
		return Fail("COAL replacement bypassed the external carbon sink ledger");
	}

	auto managedReplacement = std::make_unique<ProbeSimulation>();
	managedReplacement->SetOmniSimulationMode(OMNI_ENHANCED);
	const int managedCoal = managedReplacement->create_part(-1, 240, 180, PT_COAL);
	const double managedCarbonBefore = managedReplacement->GetOmniCarbonParcelMassKg(managedCoal) * 0.5;
	managedReplacement->SetCarbonParcelMassKg(managedCoal, managedCarbonBefore);
	if (managedReplacement->create_part(managedCoal, 240, 180, PT_BCOL) != managedCoal ||
		std::abs(managedReplacement->GetOmniCarbonParcelMassKg(managedCoal) - managedCarbonBefore) > 1.0e-15)
	{
		return Fail("COAL-to-BCOL replacement reset the authoritative carbon mass");
	}

	auto waterReplacement = std::make_unique<ProbeSimulation>();
	waterReplacement->SetOmniSimulationMode(OMNI_ENHANCED);
	const int water = waterReplacement->create_part(-1, 240, 180, PT_WATR);
	waterReplacement->BeginOmniWaterCouplingTick();
	if (water < 0 || waterReplacement->create_part(water, 240, 180, PT_WOOD) != water)
		return Fail("could not create water replacement ledger fixture");
	waterReplacement->FinishOmniWaterCouplingTick();
	if (std::abs(waterReplacement->GetOmniWaterCouplingMetrics().waterMassResidualKg) > 1.0e-15)
		return Fail("water replacement bypassed the particle-atmosphere mass transfer");

	auto managedWaterReplacement = std::make_unique<ProbeSimulation>();
	managedWaterReplacement->SetOmniSimulationMode(OMNI_ENHANCED);
	const int managedWater = managedWaterReplacement->create_part(-1, 240, 180, PT_WATR);
	const double managedWaterBefore = managedWaterReplacement->GetOmniWaterParcelMassKg(managedWater) * 0.5;
	managedWaterReplacement->SetWaterParcelMassKg(managedWater, managedWaterBefore);
	if (managedWaterReplacement->create_part(managedWater, 240, 180, PT_ICEI) != managedWater ||
		std::abs(managedWaterReplacement->GetOmniWaterParcelMassKg(managedWater) - managedWaterBefore) > 1.0e-15)
	{
		return Fail("WATR-to-ICEI replacement reset the authoritative water mass");
	}

	auto save = normal.simulation->Save(true, RES.OriginRect());
	if (!save)
		return Fail("Enhanced combustion save creation failed");
	auto bytes = save->Serialise().second;
	if (bytes.empty())
		return Fail("Enhanced combustion OPS serialization failed");
	GameSave parsed(bytes);
	auto restored = Simulation::Factory();
	restored->SetOmniSimulationMode(parsed.omniSimulationMode);
	restored->Load(&parsed, true, { 0, 0 });
	int restoredCoal = -1;
	for (int index = 0; index < restored->parts.active; ++index)
		if (restored->parts[index].type == PT_COAL || restored->parts[index].type == PT_BCOL)
		{
			restoredCoal = index;
			break;
		}
	if (restoredCoal < 0 ||
		std::abs(restored->GetOmniCarbonParcelMassKg(restoredCoal) - normal.carbonAfter) > 1.0e-12)
	{
		return Fail("carbon sidecar mass did not survive OPS round trip");
	}

	std::cout << "omni_combustion_probe_pass=true\n";
	std::cout << "normal_oxygen_consumed_kg=" << normal.metrics.oxygenConsumedKg << '\n';
	std::cout << "normal_carbon_dioxide_produced_kg=" << normal.metrics.carbonDioxideProducedKg << '\n';
	std::cout << "low_oxygen_consumed_kg=" << low.metrics.oxygenConsumedKg << '\n';
	std::cout << "high_oxygen_consumed_kg=" << high.metrics.oxygenConsumedKg << '\n';
	std::cout << "chemical_energy_released_j=" << normal.metrics.chemicalEnergyReleasedJ << '\n';
	std::cout << "mass_residual_kg=" << normal.metrics.massResidualKg << '\n';
	std::cout << "total_mass_balance_residual_kg=" << normal.metrics.totalMassBalanceResidualKg << '\n';
	std::cout << "carbon_atom_residual_mol=" << normal.metrics.carbonAtomResidualMol << '\n';
	std::cout << "total_carbon_atom_balance_residual_mol=" <<
		normal.metrics.totalCarbonAtomBalanceResidualMol << '\n';
	std::cout << "oxygen_atom_residual_mol=" << normal.metrics.oxygenAtomResidualMol << '\n';
	std::cout << "energy_residual_j=" << normal.metrics.energyResidualJ << '\n';
	std::cout << "carbon_mass_storage=dedicated_double_sidecar\n";
	std::cout << "classic_reaction_transactions=0\n";
	std::cout << "classic_to_enhanced_water_carbon_initialized=true\n";
	std::cout << "classic_roundtrip_sidecars_preserved=true\n";
	std::cout << "type_change_carbon_sink_kg=" << lifecycleMetrics.externalCarbonMassSinkKg << '\n';
	std::cout << "replacement_carbon_sink_kg=" << replacementMetrics.externalCarbonMassSinkKg << '\n';
	std::cout << "managed_replacement_mass_preserved=true\n";
	std::cout << "water_replacement_mass_conserved=true\n";
	std::cout << "managed_water_replacement_mass_preserved=true\n";
	std::cout << "save_roundtrip=true\n";
	return 0;
}
