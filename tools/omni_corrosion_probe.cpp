#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/ElementClasses.h"
#include "simulation/OmniAtmosphere.h"
#include "simulation/OmniCorrosion.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"
#include "simulation/SnapshotDelta.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>

namespace
{
class ProbeSimulation : public Simulation
{
public:
	using Simulation::SetOmniCorrosionState;
	void UpdateParticles(int, int) override {}
};

int Fail(std::string_view message)
{
	std::cerr << "omni-corrosion-probe: FAIL " << message << '\n';
	return 1;
}

std::unique_ptr<ProbeSimulation> Enhanced()
{
	auto simulation = std::make_unique<ProbeSimulation>();
	simulation->gravityMode = GRAV_OFF;
	simulation->SetEdgeMode(EDGE_SOLID);
	simulation->SetOmniSimulationMode(OMNI_ENHANCED);
	return simulation;
}

int CreateIron(ProbeSimulation &simulation, int x, int y, float temperature = 293.15f)
{
	const int iron = simulation.create_part(-1, x, y, PT_IRON);
	if (iron >= 0)
		simulation.parts[iron].temp = temperature;
	return iron;
}

void DryAtmosphere(ProbeSimulation &simulation, int x, int y)
{
	simulation.omniAtmosphere->SetSpeciesMassFractions(x / CELL, y / CELL,
		{ 0.767, 0.233, 0.0, 0.0, 0.0 });
}

void StepCorrosion(ProbeSimulation &simulation, int iron, int x, int y, int ticks)
{
	for (int tick = 0; tick < ticks && simulation.parts[iron].type == PT_IRON; ++tick)
		simulation.UpdateOmniIronCorrosion(iron, x, y);
}
}

int main()
{
	GlobalPrefs globalPrefs;
	SimulationData simulationData;

	auto dry = Enhanced();
	const int dryIron = CreateIron(*dry, 120, 120);
	if (dryIron < 0)
		return Fail("could not create dry fixture");
	DryAtmosphere(*dry, 120, 120);
	StepCorrosion(*dry, dryIron, 120, 120, 200);
	if (dry->GetOmniCorrosionProgress(dryIron) != 0.0)
		return Fail("dry iron accumulated corrosion");

	auto wet = Enhanced();
	const int wetIron = CreateIron(*wet, 160, 120);
	const int wetWater = wet->create_part(-1, 161, 120, PT_WATR);
	if (wetIron < 0 || wetWater < 0)
		return Fail("could not create wet fixture");
	DryAtmosphere(*wet, 160, 120);
	StepCorrosion(*wet, wetIron, 160, 120, 20);
	const double wetProgress = wet->GetOmniCorrosionProgress(wetIron);
	const double wetPassivation = wet->GetOmniCorrosionPassivation(wetIron);
	if (!(wetProgress > 0.0) || !(wetProgress < 1.0) || !(wetPassivation > 0.0))
		return Fail("wet oxygenated iron did not accumulate a passive corrosion process");

	auto salted = Enhanced();
	const int saltIron = CreateIron(*salted, 200, 120);
	const int saltWater = salted->create_part(-1, 201, 120, PT_SLTW);
	if (saltIron < 0 || saltWater < 0)
		return Fail("could not create chloride fixture");
	DryAtmosphere(*salted, 200, 120);
	StepCorrosion(*salted, saltIron, 200, 120, 20);
	const double saltProgress = salted->GetOmniCorrosionProgress(saltIron);
	if (!(saltProgress > wetProgress * 2.0))
		return Fail("NaCl did not accelerate corrosion over clean water");

	auto hot = Enhanced();
	const int hotIron = CreateIron(*hot, 240, 120, 333.15f);
	const int hotWater = hot->create_part(-1, 241, 120, PT_WATR);
	if (hotIron < 0 || hotWater < 0)
		return Fail("could not create temperature fixture");
	DryAtmosphere(*hot, 240, 120);
	StepCorrosion(*hot, hotIron, 240, 120, 20);
	if (!(hot->GetOmniCorrosionProgress(hotIron) > wetProgress))
		return Fail("temperature factor did not accelerate wet corrosion");

	auto protectedFixture = Enhanced();
	const int protectedIron = CreateIron(*protectedFixture, 280, 120);
	const int protectedWater = protectedFixture->create_part(-1, 281, 120, PT_WATR);
	const int zinc = protectedFixture->create_part(-1, 280, 121, PT_ZINC);
	if (protectedIron < 0 || protectedWater < 0 || zinc < 0)
		return Fail("could not create zinc-protection fixture");
	DryAtmosphere(*protectedFixture, 280, 120);
	StepCorrosion(*protectedFixture, protectedIron, 280, 120, 200);
	if (protectedFixture->GetOmniCorrosionProgress(protectedIron) != 0.0)
		return Fail("zinc protection did not pause wet iron corrosion");

	const double saveProgress = saltProgress;
	auto save = salted->Save(true, RES.OriginRect());
	if (!save || !save->hasOmniCorrosionState ||
		save->omniCorrosionStateVersion != GameSave::OmniCorrosionStateVersion)
		return Fail("Enhanced save omitted corrosion state");
	const auto bytes = save->Serialise().second;
	if (bytes.empty())
		return Fail("corrosion OPS serialization failed");
	GameSave parsed(bytes);
	if (!parsed.hasOmniCorrosionState ||
		parsed.omniCorrosionProgress.size() != static_cast<size_t>(parsed.particlesCount) ||
		parsed.omniCorrosionPassivation.size() != static_cast<size_t>(parsed.particlesCount))
		return Fail("corrosion OPS parsing failed");
	GameSave malformed = parsed;
	bool corrupted = false;
	for (double &value : malformed.omniCorrosionProgress)
		if (value > 0.0)
		{
			value = 1.5;
			corrupted = true;
			break;
		}
	if (!corrupted || malformed.Serialise().first)
		return Fail("out-of-range corrosion progress payload was not rejected");
	auto restored = Simulation::Factory();
	restored->SetOmniSimulationMode(parsed.omniSimulationMode);
	restored->Load(&parsed, true, { 0, 0 });
	int restoredIron = -1;
	for (int index = 0; index < restored->parts.active; ++index)
		if (restored->parts[index].type == PT_IRON)
		{
			restoredIron = index;
			break;
		}
	if (restoredIron < 0 || std::abs(restored->GetOmniCorrosionProgress(restoredIron) - saveProgress) > 1.0e-15)
		return Fail("corrosion state did not survive OPS round trip");

	auto before = salted->CreateSnapshot();
	salted->SetOmniCorrosionState(saltIron, saveProgress + 0.1,
		salted->GetOmniCorrosionPassivation(saltIron) + 0.05);
	auto after = salted->CreateSnapshot();
	auto delta = SnapshotDelta::FromSnapshots(*before, *after);
	auto forward = delta->Forward(*before);
	auto reverse = delta->Restore(*after);
	if (forward->OmniCorrosionProgress != after->OmniCorrosionProgress ||
		forward->OmniCorrosionPassivation != after->OmniCorrosionPassivation ||
		reverse->OmniCorrosionProgress != before->OmniCorrosionProgress ||
		reverse->OmniCorrosionPassivation != before->OmniCorrosionPassivation)
		return Fail("corrosion sidecars did not survive SnapshotDelta");
	salted->Restore(*before);
	if (std::abs(salted->GetOmniCorrosionProgress(saltIron) - saveProgress) > 1.0e-15)
		return Fail("corrosion state did not survive Snapshot restore");

	auto completion = Enhanced();
	const int completionIron = CreateIron(*completion, 320, 120, 333.15f);
	const int completionSalt = completion->create_part(-1, 321, 120, PT_SLTW);
	if (completionIron < 0 || completionSalt < 0)
		return Fail("could not create completion fixture");
	DryAtmosphere(*completion, 320, 120);
	StepCorrosion(*completion, completionIron, 320, 120, 500);
	if (completion->parts[completionIron].type != PT_BMTL || completion->parts[completionIron].tmp != 20)
		return Fail("accumulated corrosion did not reach the Legacy visual endpoint");

	auto classic = Simulation::Factory();
	const int classicIron = classic->create_part(-1, 360, 120, PT_IRON);
	const int liquidOxygen = classic->create_part(-1, 361, 120, PT_LO2);
	if (classicIron < 0 || liquidOxygen < 0 ||
		classic->UpdateOmniIronCorrosion(classicIron, 360, 120))
		return Fail("Classic entered the Enhanced corrosion path");
	classic->BeforeSim(true);
	classic->UpdateParticles(0, classic->parts.active);
	classic->AfterSim();
	if (classic->parts[classicIron].type != PT_BMTL)
		return Fail("Classic official immediate LO2 corrosion path changed");
	auto classicSave = classic->Save(true, RES.OriginRect());
	if (!classicSave || classicSave->hasOmniCorrosionState)
		return Fail("Classic save emitted Enhanced corrosion metadata");

	std::cout << "omni_corrosion_probe_pass=true\n";
	std::cout << "dry_progress=" << dry->GetOmniCorrosionProgress(dryIron) << '\n';
	std::cout << "wet_progress=" << wetProgress << '\n';
	std::cout << "salt_progress=" << saltProgress << '\n';
	std::cout << "hot_progress=" << hot->GetOmniCorrosionProgress(hotIron) << '\n';
	std::cout << "wet_passivation=" << wetPassivation << '\n';
	std::cout << "zinc_protected_progress=0\n";
	std::cout << "ops_roundtrip_progress=" << saveProgress << '\n';
	std::cout << "classic_lo2_path=true\n";
	return 0;
}
