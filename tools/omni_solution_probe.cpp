#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/ElementClasses.h"
#include "simulation/OmniSolution.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"
#include "simulation/SnapshotDelta.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <limits>
#include <string_view>

namespace
{
class ProbeSimulation : public Simulation
{
public:
	using Simulation::BeginOmniSolutionTick;
	using Simulation::FinishOmniSolutionTick;
	using Simulation::SetOmniSolutionMassesKg;
	void UpdateParticles(int, int) override {}
};

int Fail(std::string_view message)
{
	std::cerr << "omni-solution-probe: FAIL " << message << '\n';
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
}

int main()
{
	GlobalPrefs globalPrefs;
	SimulationData simulationData;

	auto dissolution = EnhancedSimulation();
	const int water = dissolution->create_part(-1, 200, 180, PT_WATR);
	const int salt = dissolution->create_part(-1, 201, 180, PT_SALT);
	if (water < 0 || salt < 0)
		return Fail("could not create dissolution fixture");
	const double initialWaterKg = dissolution->GetOmniWaterParcelMassKg(water);
	const double initialSaltKg = dissolution->GetOmniSolutionSoluteMassKg(salt);
	AdvanceOneTick(*dissolution);
	int solution = -1, remainingSalt = -1;
	for (int index = 0; index < dissolution->parts.active; ++index)
	{
		if (dissolution->parts[index].type == PT_SLTW)
			solution = index;
		else if (dissolution->parts[index].type == PT_SALT)
			remainingSalt = index;
	}
	const auto dissolutionMetrics = dissolution->GetOmniSolutionMetrics();
	if (solution < 0 || remainingSalt < 0 ||
		std::abs(dissolution->GetOmniSolutionSolventMassKg(solution) - initialWaterKg) > 1.0e-15 ||
		!(dissolution->GetOmniSolutionSoluteMassKg(solution) > 0.0) ||
		!(dissolution->GetOmniSolutionSoluteMassKg(remainingSalt) < initialSaltKg) ||
		std::abs(dissolutionMetrics.solventMassResidualKg) > 1.0e-12 ||
		std::abs(dissolutionMetrics.soluteMassResidualKg) > 1.0e-12 ||
		dissolutionMetrics.dissolutionTransactions != 1 ||
		!(dissolutionMetrics.dissolvedMassKg < initialSaltKg))
	{
		return Fail("rate-limited dissolution did not conserve solvent and solute");
	}

	auto evaporation = EnhancedSimulation();
	const int hotSolution = evaporation->create_part(-1, 260, 180, PT_SLTW);
	if (hotSolution < 0)
		return Fail("could not create evaporation fixture");
	evaporation->parts[hotSolution].temp = 500.0f;
	const double evaporationSolventBefore = evaporation->GetOmniSolutionSolventMassKg(hotSolution);
	const double evaporationSoluteBefore = evaporation->GetOmniSolutionSoluteMassKg(hotSolution);
	const double concentrationBefore = OmniSolution::SoluteMassFraction(
		evaporationSolventBefore, evaporationSoluteBefore);
	AdvanceOneTick(*evaporation);
	const double evaporationSolventAfter = evaporation->GetOmniSolutionSolventMassKg(hotSolution);
	const double evaporationSoluteAfter = evaporation->GetOmniSolutionSoluteMassKg(hotSolution);
	const double concentrationAfter = OmniSolution::SoluteMassFraction(
		evaporationSolventAfter, evaporationSoluteAfter);
	const auto evaporationMetrics = evaporation->GetOmniSolutionMetrics();
	if (!(evaporationSolventAfter < evaporationSolventBefore) ||
		!(concentrationAfter > concentrationBefore) ||
		!(evaporationMetrics.transferredSolventToAtmosphereKg > 0.0) ||
		std::abs(evaporationMetrics.solventMassResidualKg) > 1.0e-12 ||
		std::abs(evaporationMetrics.soluteMassResidualKg) > 1.0e-12)
	{
		return Fail("solution evaporation did not increase concentration conservatively");
	}

	auto crystallisation = std::make_unique<ProbeSimulation>();
	crystallisation->gravityMode = GRAV_OFF;
	crystallisation->SetEdgeMode(EDGE_SOLID);
	crystallisation->SetOmniSimulationMode(OMNI_ENHANCED);
	const int saturated = crystallisation->create_part(-1, 320, 180, PT_SLTW);
	if (saturated < 0)
		return Fail("could not create crystallisation fixture");
	const double solventKg = crystallisation->GetOmniSolutionSolventMassKg(saturated);
	const double capacityKg = OmniSolution::MaximumDissolvedSoluteKg(
		solventKg, crystallisation->parts[saturated].temp);
	const double excessKg = 10.0 * OmniSolution::MaximumCrystallisationMassPerTickKg;
	crystallisation->SetOmniSolutionMassesKg(saturated, solventKg, capacityKg + excessKg, false);
	crystallisation->BeginOmniSolutionTick();
	crystallisation->UpdateOmniSolutionParticle(saturated, 320, 180);
	crystallisation->FinishOmniSolutionTick();
	const auto crystalMetrics = crystallisation->GetOmniSolutionMetrics();
	double crystalMassKg = 0.0;
	for (int index = 0; index < crystallisation->parts.active; ++index)
		if (index != saturated && crystallisation->parts[index].type == PT_SALT)
			crystalMassKg += crystallisation->GetOmniSolutionSoluteMassKg(index);
	if (crystalMetrics.crystallisationTransactions != 1 ||
		!(crystalMassKg > 0.0) || !(crystalMassKg < excessKg) ||
		std::abs(crystalMassKg - OmniSolution::MaximumCrystallisationMassPerTickKg) > 1.0e-15 ||
		std::abs(crystalMetrics.soluteMassResidualKg) > 1.0e-12)
	{
		return Fail("supersaturation did not produce rate-limited conservative crystals");
	}

	auto save = evaporation->Save(true, RES.OriginRect());
	if (!save || !save->hasOmniSolutionState ||
		save->omniSolutionStateVersion != GameSave::OmniSolutionStateVersion)
		return Fail("Enhanced save omitted solution state");
	const auto bytes = save->Serialise().second;
	if (bytes.empty())
		return Fail("solution OPS serialization failed");
	GameSave parsed(bytes);
	if (!parsed.hasOmniSolutionState ||
		parsed.omniSolutionSolventMassKg.size() != static_cast<size_t>(parsed.particlesCount) ||
		parsed.omniSolutionSoluteMassKg.size() != static_cast<size_t>(parsed.particlesCount))
		return Fail("solution OPS parsing failed");
	GameSave transformed = parsed;
	transformed.Transform(Mat2<int>{ 1, 0, 0, 1 }, { 0, 0 });
	if (!transformed.hasOmniSolutionState ||
		transformed.omniSolutionSolventMassKg.size() != static_cast<size_t>(transformed.particlesCount) ||
		transformed.omniSolutionSoluteMassKg.size() != static_cast<size_t>(transformed.particlesCount))
		return Fail("solution sidecars did not survive identity transform");
	GameSave malformed = parsed;
	bool corrupted = false;
	for (double &massKg : malformed.omniSolutionSoluteMassKg)
		if (massKg > 0.0)
		{
			massKg = std::numeric_limits<double>::quiet_NaN();
			corrupted = true;
			break;
		}
	if (!corrupted || malformed.Serialise().first)
		return Fail("non-finite solution payload was not rejected");
	auto restored = Simulation::Factory();
	restored->SetOmniSimulationMode(parsed.omniSimulationMode);
	restored->Load(&parsed, true, { 0, 0 });
	int restoredSolution = -1;
	for (int index = 0; index < restored->parts.active; ++index)
		if (restored->parts[index].type == PT_SLTW)
		{
			restoredSolution = index;
			break;
		}
	if (restoredSolution < 0 ||
		std::abs(restored->GetOmniSolutionSolventMassKg(restoredSolution) - evaporationSolventAfter) > 1.0e-15 ||
		std::abs(restored->GetOmniSolutionSoluteMassKg(restoredSolution) - evaporationSoluteAfter) > 1.0e-15)
		return Fail("solution masses did not survive OPS round trip");

	auto before = evaporation->CreateSnapshot();
	evaporation->parts[hotSolution].temp = 300.0f;
	auto after = evaporation->CreateSnapshot();
	auto delta = SnapshotDelta::FromSnapshots(*before, *after);
	auto deltaForward = delta->Forward(*before);
	auto deltaRestore = delta->Restore(*after);
	if (deltaForward->OmniSolutionSolventMassKg != after->OmniSolutionSolventMassKg ||
		deltaForward->OmniSolutionSoluteMassKg != after->OmniSolutionSoluteMassKg ||
		deltaRestore->OmniSolutionSolventMassKg != before->OmniSolutionSolventMassKg ||
		deltaRestore->OmniSolutionSoluteMassKg != before->OmniSolutionSoluteMassKg)
		return Fail("solution sidecars did not survive SnapshotDelta forward/restore");
	evaporation->Restore(*before);
	if (std::abs(evaporation->GetOmniSolutionSolventMassKg(hotSolution) - evaporationSolventAfter) > 1.0e-15)
		return Fail("solution masses did not survive Snapshot restore");
	evaporation->Restore(*after);
	if (std::abs(evaporation->GetOmniSolutionSoluteMassKg(hotSolution) - evaporationSoluteAfter) > 1.0e-15)
		return Fail("solution masses did not survive Snapshot forward restore");

	auto classic = Simulation::Factory();
	const int classicSalt = classic->create_part(-1, 200, 180, PT_SALT);
	const int classicWater = classic->create_part(-1, 201, 180, PT_WATR);
	if (classicSalt < 0 || classicWater < 0 ||
		classic->UpdateOmniSolutionParticle(classicSalt, 200, 180) ||
		classic->GetOmniSolutionSoluteMassKg(classicSalt) != 0.0)
		return Fail("Classic entered the Enhanced solution path");
	auto classicSave = classic->Save(true, RES.OriginRect());
	if (!classicSave || classicSave->hasOmniSolutionState)
		return Fail("Classic save emitted Enhanced solution metadata");

	std::cout << "omni_solution_probe_pass=true\n";
	std::cout << "dissolved_mass_kg=" << dissolutionMetrics.dissolvedMassKg << '\n';
	std::cout << "evaporated_solvent_mass_kg=" << evaporationMetrics.transferredSolventToAtmosphereKg << '\n';
	std::cout << "concentration_before=" << concentrationBefore << '\n';
	std::cout << "concentration_after=" << concentrationAfter << '\n';
	std::cout << "crystallised_mass_kg=" << crystalMassKg << '\n';
	std::cout << "solute_mass_residual_kg=" << crystalMetrics.soluteMassResidualKg << '\n';
	std::cout << "classic_solution_active=false\n";
	return 0;
}
