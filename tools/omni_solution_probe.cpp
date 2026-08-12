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
	using Simulation::SetOmniSolutionNeutralSaltMassKg;
	using Simulation::TotalOmniSolutionSoluteMassKg;
	using Simulation::TotalOmniSolutionSolventMassKg;
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

	auto neutralisation = EnhancedSimulation();
	const int acid = neutralisation->create_part(-1, 360, 180, PT_ACID);
	const int base = neutralisation->create_part(-1, 361, 180, PT_BASE);
	if (acid < 0 || base < 0)
		return Fail("could not create neutralisation fixture");
	const double acidBeforeKg = neutralisation->GetOmniSolutionSoluteMassKg(acid);
	const double baseBeforeKg = neutralisation->GetOmniSolutionSoluteMassKg(base);
	const double neutralisationMassBefore =
		neutralisation->GetOmniSolutionSolventMassKg(acid) + acidBeforeKg +
		neutralisation->GetOmniSolutionSolventMassKg(base) + baseBeforeKg;
	const double temperatureBefore = neutralisation->parts[acid].temp;
	AdvanceOneTick(*neutralisation);
	const auto neutralMetrics = neutralisation->GetOmniSolutionMetrics();
	const double neutralisationMassAfter =
		neutralisation->GetOmniSolutionSolventMassKg(acid) +
		neutralisation->GetOmniSolutionSoluteMassKg(acid) +
		neutralisation->GetOmniSolutionNeutralSaltMassKg(acid) +
		neutralisation->GetOmniSolutionSolventMassKg(base) +
		neutralisation->GetOmniSolutionSoluteMassKg(base) +
		neutralisation->GetOmniSolutionNeutralSaltMassKg(base);
	if (neutralMetrics.neutralisationTransactions != 1 ||
		!(neutralMetrics.neutralisedAcidMassKg > 0.0) ||
		!(neutralMetrics.neutralisedBaseMassKg > 0.0) ||
		!(neutralMetrics.neutralSaltProducedKg > 0.0) ||
		!(neutralMetrics.neutralisationWaterProducedKg > 0.0) ||
		!(neutralMetrics.neutralisationEnergyReleasedJ > 0.0) ||
		!(neutralisation->parts[acid].temp > temperatureBefore) ||
		std::abs(neutralisationMassAfter - neutralisationMassBefore) > 1.0e-12 ||
		std::abs(neutralMetrics.totalSolutionMassResidualKg) > 1.0e-12)
		return Fail("rate-limited acid/base neutralisation did not conserve total mass and release heat");

	auto unequal = std::make_unique<ProbeSimulation>();
	unequal->gravityMode = GRAV_OFF;
	unequal->SetEdgeMode(EDGE_SOLID);
	unequal->SetOmniSimulationMode(OMNI_ENHANCED);
	const int limitingAcid = unequal->create_part(-1, 380, 180, PT_ACID);
	const int excessBase = unequal->create_part(-1, 381, 180, PT_BASE);
	if (limitingAcid < 0 || excessBase < 0)
		return Fail("could not create unequal neutralisation fixture");
	const double limitingAcidKg =
		OmniSolution::MaximumNeutralisationMolesPerTransaction *
		OmniSolution::HydrogenChlorideMolarMassKgPerMol * 0.5;
	const double excessBaseBeforeKg = unequal->GetOmniSolutionSoluteMassKg(excessBase);
	unequal->SetOmniSolutionMassesKg(limitingAcid,
		unequal->GetOmniSolutionSolventMassKg(limitingAcid), limitingAcidKg, false);
	const double unequalMassBefore = unequal->TotalOmniSolutionSolventMassKg() +
		unequal->TotalOmniSolutionSoluteMassKg();
	unequal->BeginOmniSolutionTick();
	unequal->UpdateOmniSolutionParticle(limitingAcid, 380, 180);
	unequal->FinishOmniSolutionTick();
	const auto unequalMetrics = unequal->GetOmniSolutionMetrics();
	const double unequalMassAfter = unequal->TotalOmniSolutionSolventMassKg() +
		unequal->TotalOmniSolutionSoluteMassKg();
	if (unequalMetrics.neutralisationTransactions != 1 ||
		unequal->parts[limitingAcid].type != PT_SLTW ||
		unequal->parts[excessBase].type != PT_BASE ||
		unequal->GetOmniSolutionSoluteMassKg(limitingAcid) != 0.0 ||
		!(unequal->GetOmniSolutionSoluteMassKg(excessBase) > 0.0) ||
		!(unequal->GetOmniSolutionSoluteMassKg(excessBase) < excessBaseBeforeKg) ||
		std::abs(unequalMassAfter - unequalMassBefore) > 1.0e-12 ||
		std::abs(unequalMetrics.totalSolutionMassResidualKg) > 1.0e-12)
		return Fail("unequal neutralisation did not retain excess base conservatively");

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
	if (parsed.omniSolutionNeutralSaltMassKg.size() != static_cast<size_t>(parsed.particlesCount))
		return Fail("solution OPS v2 omitted neutral salt state");
	auto neutralSave = neutralisation->Save(true, RES.OriginRect());
	if (!neutralSave || !neutralSave->hasOmniSolutionState)
		return Fail("neutralisation save omitted solution state");
	GameSave parsedNeutral(neutralSave->Serialise().second);
	double parsedNeutralSaltKg = 0.0;
	for (double massKg : parsedNeutral.omniSolutionNeutralSaltMassKg)
		parsedNeutralSaltKg += massKg;
	if (std::abs(parsedNeutralSaltKg - neutralMetrics.neutralSaltProducedKg) > 1.0e-12)
		return Fail("solution OPS v2 did not preserve neutral salt mass");
	auto restoredNeutral = Simulation::Factory();
	restoredNeutral->SetOmniSimulationMode(parsedNeutral.omniSimulationMode);
	restoredNeutral->Load(&parsedNeutral, true, { 0, 0 });
	double restoredNeutralSaltKg = 0.0;
	for (int index = 0; index < restoredNeutral->parts.active; ++index)
		restoredNeutralSaltKg += restoredNeutral->GetOmniSolutionNeutralSaltMassKg(index);
	if (std::abs(restoredNeutralSaltKg - neutralMetrics.neutralSaltProducedKg) > 1.0e-12)
		return Fail("neutral salt mass did not survive OPS load");
	GameSave legacy = parsed;
	legacy.omniSolutionStateVersion = GameSave::OmniSolutionLegacyStateVersion;
	legacy.omniSolutionNeutralSaltMassKg.assign(legacy.particlesCount, 0.0);
	const auto legacyBytes = legacy.Serialise().second;
	if (legacyBytes.empty())
		return Fail("solution OPS v1 compatibility fixture could not be serialized");
	GameSave parsedLegacy(legacyBytes);
	if (!parsedLegacy.hasOmniSolutionState ||
		parsedLegacy.omniSolutionStateVersion != GameSave::OmniSolutionLegacyStateVersion ||
		parsedLegacy.omniSolutionNeutralSaltMassKg.size() != static_cast<size_t>(parsedLegacy.particlesCount))
		return Fail("solution OPS v1 did not migrate to zero neutral salt state");
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
	GameSave negativeNeutralSalt = parsedNeutral;
	corrupted = false;
	for (double &massKg : negativeNeutralSalt.omniSolutionNeutralSaltMassKg)
		if (massKg > 0.0)
		{
			massKg = -massKg;
			corrupted = true;
			break;
		}
	if (!corrupted || negativeNeutralSalt.Serialise().first)
		return Fail("negative neutral salt payload was not rejected");
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

	auto before = neutralisation->CreateSnapshot();
	const double snapshotNeutralSaltBefore =
		neutralisation->GetOmniSolutionNeutralSaltMassKg(acid);
	auto after = neutralisation->CreateSnapshot();
	if (acid >= static_cast<int>(after->OmniSolutionNeutralSaltMassKg.size()))
		return Fail("solution snapshot omitted neutral salt channel");
	after->OmniSolutionNeutralSaltMassKg[acid] = snapshotNeutralSaltBefore +
		OmniSolution::MaximumCrystallisationMassPerTickKg;
	auto delta = SnapshotDelta::FromSnapshots(*before, *after);
	auto deltaForward = delta->Forward(*before);
	auto deltaRestore = delta->Restore(*after);
	if (deltaForward->OmniSolutionSolventMassKg != after->OmniSolutionSolventMassKg ||
		deltaForward->OmniSolutionSoluteMassKg != after->OmniSolutionSoluteMassKg ||
		deltaForward->OmniSolutionNeutralSaltMassKg != after->OmniSolutionNeutralSaltMassKg ||
		deltaRestore->OmniSolutionSolventMassKg != before->OmniSolutionSolventMassKg ||
		deltaRestore->OmniSolutionSoluteMassKg != before->OmniSolutionSoluteMassKg ||
		deltaRestore->OmniSolutionNeutralSaltMassKg != before->OmniSolutionNeutralSaltMassKg)
		return Fail("solution sidecars did not survive SnapshotDelta forward/restore");
	neutralisation->Restore(*before);
	if (std::abs(neutralisation->GetOmniSolutionNeutralSaltMassKg(acid) -
		snapshotNeutralSaltBefore) > 1.0e-15)
		return Fail("solution masses did not survive Snapshot restore");
	neutralisation->Restore(*after);
	if (std::abs(neutralisation->GetOmniSolutionNeutralSaltMassKg(acid) -
		snapshotNeutralSaltBefore - OmniSolution::MaximumCrystallisationMassPerTickKg) > 1.0e-15)
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
	std::cout << "neutralisation_transactions=" << neutralMetrics.neutralisationTransactions << '\n';
	std::cout << "neutralised_acid_mass_kg=" << neutralMetrics.neutralisedAcidMassKg << '\n';
	std::cout << "neutralised_base_mass_kg=" << neutralMetrics.neutralisedBaseMassKg << '\n';
	std::cout << "neutral_salt_produced_kg=" << neutralMetrics.neutralSaltProducedKg << '\n';
	std::cout << "neutralisation_water_produced_kg=" << neutralMetrics.neutralisationWaterProducedKg << '\n';
	std::cout << "neutralisation_energy_released_j=" << neutralMetrics.neutralisationEnergyReleasedJ << '\n';
	std::cout << "total_solution_mass_residual_kg=" << neutralMetrics.totalSolutionMassResidualKg << '\n';
	std::cout << "unequal_excess_base_retained=true\n";
	std::cout << "classic_solution_active=false\n";
	return 0;
}
