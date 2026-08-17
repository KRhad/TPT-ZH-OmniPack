#include "PowderToySDL.h"
#include "Format.h"
#include "X86KillDenormals.h"
#include "prefs/GlobalPrefs.h"
#include "client/Client.h"
#include "client/GameSave.h"
#include "client/SaveFile.h"
#include "client/SaveInfo.h"
#include "client/http/requestmanager/RequestManager.h"
#include "client/http/GetSaveRequest.h"
#include "client/http/GetSaveDataRequest.h"
#include "common/platform/Platform.h"
#include "graphics/Graphics.h"
#include "simulation/SaveRenderer.h"
#include "simulation/ElementClasses.h"
#include "simulation/OmniAtmosphere.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"
#include "simulation/OmniCompute.h"
#include "common/tpt-rand.h"
#include "gui/game/Favorite.h"
#include "gui/Style.h"
#include "gui/game/GameController.h"
#include "gui/game/GameView.h"
#include "gui/game/IntroText.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/interface/Engine.h"
#include "gui/interface/TextWrapper.h"
#include "Config.h"
#include "common/Localization.h"
#include "SimulationConfig.h"
#include <algorithm>
#include <optional>
#include <climits>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <csignal>
#include "common/platform/SDLCompat.h"
#if TPT_SDL3
# define SDL_MAIN_HANDLED
# include <SDL3/SDL_main.h>
#endif
#include <exception>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace
{
struct PortableRuntimeEvidence
{
	std::string runId;
	std::string candidateSha256;
	bool launchPassed = false;
	bool fixtureCreated = false;
	bool initialSimulatePassed = false;
	bool initialSavePassed = false;
	bool initialParsePassed = false;
	bool initialReloadPassed = false;
	bool initialStateValidatePassed = false;
	bool postStepSimulatePassed = false;
	bool postStepFinitePassed = false;
	bool postStepInventoryPassed = false;
	bool postStepAtmospherePassed = false;
	bool postStepSavePassed = false;
	bool postStepParsePassed = false;
	bool postStepReloadPassed = false;
	bool postStepRoundtripPassed = false;
	bool enhancedMode = false;
	bool omniStatePresent = false;
	bool waterSidecarRoundtrip = false;
	bool runtimeCompletionReached = false;
	int initialParticleCount = -1;
	int postStepParticleCount = -1;
	int finalParticleCount = -1;
	long long atmosphereNonFiniteCells = -1;

	bool SaveReloadPassed() const
	{
		return initialSavePassed && initialParsePassed && initialReloadPassed &&
			postStepSavePassed && postStepParsePassed && postStepReloadPassed;
	}

	bool StateValidatePassed() const
	{
		return initialStateValidatePassed && postStepFinitePassed &&
			postStepInventoryPassed && postStepAtmospherePassed &&
			postStepRoundtripPassed;
	}

	bool Passed() const
	{
		return !runId.empty() && candidateSha256.size() == 64 && launchPassed &&
			fixtureCreated && initialSimulatePassed &&
			SaveReloadPassed() && StateValidatePassed() && postStepSimulatePassed &&
			enhancedMode && omniStatePresent && waterSidecarRoundtrip &&
			runtimeCompletionReached;
	}
};

bool PortableFinite(float value)
{
	uint32_t bits = 0;
	static_assert(sizeof(bits) == sizeof(value));
	std::memcpy(&bits, &value, sizeof(bits));
	return (bits & UINT32_C(0x7F800000)) != UINT32_C(0x7F800000);
}

bool PortableFinite(double value)
{
	uint64_t bits = 0;
	static_assert(sizeof(bits) == sizeof(value));
	std::memcpy(&bits, &value, sizeof(bits));
	return (bits & UINT64_C(0x7FF0000000000000)) != UINT64_C(0x7FF0000000000000);
}

bool PortableNearlyEqual(double lhs, double rhs, double absoluteTolerance, double relativeTolerance)
{
	if (!PortableFinite(lhs) || !PortableFinite(rhs))
		return false;
	return std::abs(lhs - rhs) <= absoluteTolerance +
		relativeTolerance * std::max(std::abs(lhs), std::abs(rhs));
}

int PortableLiveParticleCount(const Simulation &simulation)
{
	int count = 0;
	for (int index = 0; index < simulation.parts.active; ++index)
		if (simulation.parts[index].type)
			++count;
	return count;
}

bool PortableFixtureInventoryPresent(const Simulation &simulation)
{
	bool dustPresent = false;
	bool waterPresent = false;
	for (int index = 0; index < simulation.parts.active; ++index)
	{
		const auto &particle = simulation.parts[index];
		if (!particle.type)
			continue;
		if (particle.type == PT_DUST)
			dustPresent = true;
		const int carrierType = particle.type == PT_SPRK ? particle.ctype : particle.type;
		if ((carrierType == PT_WATR || carrierType == PT_ICEI || carrierType == PT_WTRV) &&
			PortableFinite(simulation.GetOmniWaterParcelMassKg(index)) &&
			simulation.GetOmniWaterParcelMassKg(index) > 0.0 &&
			PortableFinite(simulation.GetOmniWaterParcelSpecificEnthalpyJPerKg(index)) &&
			simulation.GetOmniWaterParcelSpecificEnthalpyJPerKg(index) > 0.0)
		{
			waterPresent = true;
		}
	}
	return dustPresent && waterPresent;
}

bool PortableParticleStateFinite(const Simulation &simulation)
{
	if (simulation.parts.active < 0 || simulation.parts.active > NPART)
		return false;
	for (int index = 0; index < simulation.parts.active; ++index)
	{
		const auto &particle = simulation.parts[index];
		if (!particle.type)
			continue;
		if (particle.type < 0 || particle.type >= PT_NUM ||
			!PortableFinite(particle.x) || !PortableFinite(particle.y) ||
			!PortableFinite(particle.vx) || !PortableFinite(particle.vy) ||
			!PortableFinite(particle.temp))
		{
			return false;
		}
	}
	return true;
}

bool PortableAtmosphereStateFinite(const Simulation &simulation, long long &nonFiniteCells)
{
	nonFiniteCells = -1;
	if (!simulation.IsOmniAtmosphereActive() || !simulation.omniAtmosphere)
		return false;
	nonFiniteCells = static_cast<long long>(simulation.omniAtmosphere->NonFiniteStateCells());
	const auto minimumDensity = simulation.omniAtmosphere->MinimumDensity();
	const auto maximumDensity = simulation.omniAtmosphere->MaximumDensity();
	const auto minimumPressure = simulation.omniAtmosphere->MinimumPressure();
	const auto maximumPressure = simulation.omniAtmosphere->MaximumPressure();
	const auto minimumTemperature = simulation.omniAtmosphere->MinimumTemperature();
	const auto maximumTemperature = simulation.omniAtmosphere->MaximumTemperature();
	return nonFiniteCells == 0 && PortableFinite(minimumDensity) &&
		PortableFinite(maximumDensity) && PortableFinite(minimumPressure) &&
		PortableFinite(maximumPressure) && PortableFinite(minimumTemperature) &&
		PortableFinite(maximumTemperature) && minimumDensity > 0.0 &&
		minimumPressure > 0.0 && minimumTemperature > 0.0 &&
		minimumDensity <= maximumDensity && minimumPressure <= maximumPressure &&
		minimumTemperature <= maximumTemperature;
}

bool PortableGameSaveStateValid(const GameSave &save)
{
	if (save.missingElements || save.omniSimulationMode != OMNI_ENHANCED ||
		!save.hasOmniAtmosphereState || !save.hasOmniWaterParcelState ||
		save.omniAtmosphereStateVersion != GameSave::OmniAtmosphereStateVersion ||
		save.omniWaterParcelStateVersion != GameSave::OmniWaterParcelStateVersion ||
		(save.hasOmniCarbonParcelState &&
			save.omniCarbonParcelStateVersion != GameSave::OmniCarbonParcelStateVersion) ||
		(save.hasOmniSolutionState &&
			save.omniSolutionStateVersion != GameSave::OmniSolutionStateVersion) ||
		(save.hasOmniCorrosionState &&
			save.omniCorrosionStateVersion != GameSave::OmniCorrosionStateVersion) ||
		save.particlesCount <= 0 || save.particles.size() < static_cast<size_t>(save.particlesCount) ||
		save.blockSize.X <= 0 || save.blockSize.Y <= 0 || save.omniAtmosphereSpecies.empty())
	{
		return false;
	}
	const auto cellCount = static_cast<size_t>(save.blockSize.X) * static_cast<size_t>(save.blockSize.Y);
	const auto speciesCount = save.omniAtmosphereSpecies.size();
	if (save.omniAtmosphereSpeciesMassDensity.size() != cellCount * speciesCount ||
		save.omniAtmosphereMomentumX.size() != cellCount ||
		save.omniAtmosphereMomentumY.size() != cellCount ||
		save.omniAtmosphereTotalEnergy.size() != cellCount ||
		save.omniAtmosphereCondensedWaterDensity.size() != cellCount ||
		save.omniAtmosphereCellValid.size() != cellCount ||
		save.omniWaterParcelMassKg.size() != static_cast<size_t>(save.particlesCount) ||
		save.omniWaterParcelSpecificEnthalpyJPerKg.size() != static_cast<size_t>(save.particlesCount) ||
		(save.hasOmniCarbonParcelState &&
			save.omniCarbonParcelMassKg.size() != static_cast<size_t>(save.particlesCount)) ||
		(save.hasOmniSolutionState &&
			(save.omniSolutionSolventMassKg.size() != static_cast<size_t>(save.particlesCount) ||
			 save.omniSolutionSoluteMassKg.size() != static_cast<size_t>(save.particlesCount) ||
			 save.omniSolutionNeutralSaltMassKg.size() != static_cast<size_t>(save.particlesCount))) ||
		(save.hasOmniCorrosionState &&
			(save.omniCorrosionProgress.size() != static_cast<size_t>(save.particlesCount) ||
			 save.omniCorrosionPassivation.size() != static_cast<size_t>(save.particlesCount))) ||
		std::any_of(save.omniAtmosphereCellValid.begin(), save.omniAtmosphereCellValid.end(),
			[](unsigned char value) { return value > 1; }))
	{
		return false;
	}
	for (int index = 0; index < save.particlesCount; ++index)
	{
		const auto &particle = save.particles[index];
		if (particle.type <= 0 || particle.type >= PT_NUM ||
			!PortableFinite(particle.x) || !PortableFinite(particle.y) ||
			!PortableFinite(particle.vx) || !PortableFinite(particle.vy) ||
			!PortableFinite(particle.temp))
		{
			return false;
		}
	}
	const auto allFiniteNonNegative = [](const auto &values) {
		return std::all_of(values.begin(), values.end(), [](double value) {
			return PortableFinite(value) && value >= 0.0;
		});
	};
	const auto validUnitInterval = [](const auto &values) {
		return std::all_of(values.begin(), values.end(), [](double value) {
			return PortableFinite(value) && value >= 0.0 && value <= 1.0;
		});
	};
	return allFiniteNonNegative(save.omniAtmosphereSpeciesMassDensity) &&
		allFiniteNonNegative(save.omniAtmosphereTotalEnergy) &&
		allFiniteNonNegative(save.omniAtmosphereCondensedWaterDensity) &&
		std::all_of(save.omniAtmosphereMomentumX.begin(), save.omniAtmosphereMomentumX.end(),
			[](double value) { return PortableFinite(value); }) &&
		std::all_of(save.omniAtmosphereMomentumY.begin(), save.omniAtmosphereMomentumY.end(),
			[](double value) { return PortableFinite(value); }) &&
		allFiniteNonNegative(save.omniWaterParcelMassKg) &&
		allFiniteNonNegative(save.omniWaterParcelSpecificEnthalpyJPerKg) &&
		(!save.hasOmniCarbonParcelState || allFiniteNonNegative(save.omniCarbonParcelMassKg)) &&
		(!save.hasOmniSolutionState ||
			(allFiniteNonNegative(save.omniSolutionSolventMassKg) &&
			 allFiniteNonNegative(save.omniSolutionSoluteMassKg) &&
			 allFiniteNonNegative(save.omniSolutionNeutralSaltMassKg))) &&
		(!save.hasOmniCorrosionState ||
			(validUnitInterval(save.omniCorrosionProgress) &&
			 validUnitInterval(save.omniCorrosionPassivation)));
}

bool PortableSerializedOmniStateMatches(const GameSave &saved, const GameSave &parsed)
{
	if (!PortableGameSaveStateValid(saved) || !PortableGameSaveStateValid(parsed) ||
		saved.particlesCount != parsed.particlesCount || saved.blockSize != parsed.blockSize ||
		saved.omniSimulationMode != parsed.omniSimulationMode ||
		saved.omniAtmosphereStateVersion != parsed.omniAtmosphereStateVersion ||
		saved.omniAtmosphereSpecies != parsed.omniAtmosphereSpecies ||
		saved.omniAtmosphereCellValid != parsed.omniAtmosphereCellValid ||
		saved.omniWaterParcelStateVersion != parsed.omniWaterParcelStateVersion ||
		saved.hasOmniCarbonParcelState != parsed.hasOmniCarbonParcelState ||
		saved.omniCarbonParcelStateVersion != parsed.omniCarbonParcelStateVersion ||
		saved.hasOmniSolutionState != parsed.hasOmniSolutionState ||
		saved.omniSolutionStateVersion != parsed.omniSolutionStateVersion ||
		saved.hasOmniCorrosionState != parsed.hasOmniCorrosionState ||
		saved.omniCorrosionStateVersion != parsed.omniCorrosionStateVersion)
	{
		return false;
	}
	const auto equalDoubles = [](const auto &lhs, const auto &rhs, double absoluteTolerance) {
		if (lhs.size() != rhs.size())
			return false;
		for (size_t index = 0; index < lhs.size(); ++index)
			if (!PortableNearlyEqual(lhs[index], rhs[index], absoluteTolerance, 1.0e-9))
				return false;
		return true;
	};
	if (!equalDoubles(saved.omniAtmosphereSpeciesMassDensity,
			parsed.omniAtmosphereSpeciesMassDensity, 1.0e-12) ||
		!equalDoubles(saved.omniAtmosphereMomentumX, parsed.omniAtmosphereMomentumX, 1.0e-12) ||
		!equalDoubles(saved.omniAtmosphereMomentumY, parsed.omniAtmosphereMomentumY, 1.0e-12) ||
		!equalDoubles(saved.omniAtmosphereTotalEnergy, parsed.omniAtmosphereTotalEnergy, 1.0e-9) ||
		!equalDoubles(saved.omniAtmosphereCondensedWaterDensity,
			parsed.omniAtmosphereCondensedWaterDensity, 1.0e-12) ||
		!equalDoubles(saved.omniWaterParcelMassKg, parsed.omniWaterParcelMassKg, 1.0e-15) ||
		!equalDoubles(saved.omniWaterParcelSpecificEnthalpyJPerKg,
			parsed.omniWaterParcelSpecificEnthalpyJPerKg, 1.0e-9) ||
		(saved.hasOmniCarbonParcelState &&
			!equalDoubles(saved.omniCarbonParcelMassKg, parsed.omniCarbonParcelMassKg, 1.0e-15)) ||
		(saved.hasOmniSolutionState &&
			(!equalDoubles(saved.omniSolutionSolventMassKg,
				parsed.omniSolutionSolventMassKg, 1.0e-15) ||
			 !equalDoubles(saved.omniSolutionSoluteMassKg,
				parsed.omniSolutionSoluteMassKg, 1.0e-15) ||
			 !equalDoubles(saved.omniSolutionNeutralSaltMassKg,
				parsed.omniSolutionNeutralSaltMassKg, 1.0e-15))) ||
		(saved.hasOmniCorrosionState &&
			(!equalDoubles(saved.omniCorrosionProgress,
				parsed.omniCorrosionProgress, 1.0e-12) ||
			 !equalDoubles(saved.omniCorrosionPassivation,
				parsed.omniCorrosionPassivation, 1.0e-12))))
	{
		return false;
	}
	for (int index = 0; index < saved.particlesCount; ++index)
	{
		const auto &lhs = saved.particles[index];
		const auto &rhs = parsed.particles[index];
		if (lhs.type != rhs.type || !PortableNearlyEqual(lhs.x, rhs.x, 0.500001, 0.0) ||
			!PortableNearlyEqual(lhs.y, rhs.y, 0.500001, 0.0) ||
			!PortableNearlyEqual(lhs.temp, rhs.temp, 0.500001, 0.0))
		{
			return false;
		}
	}
	return true;
}

bool PortableLoadedOmniStateMatches(const GameSave &parsed, const Simulation &simulation)
{
	if (!PortableGameSaveStateValid(parsed) || !simulation.IsOmniAtmosphereActive() ||
		!simulation.omniAtmosphere || PortableLiveParticleCount(simulation) != parsed.particlesCount ||
		parsed.omniAtmosphereSpecies.size() != simulation.omniAtmosphere->SpeciesCount() ||
		parsed.omniAtmosphereCellValid.size() != simulation.omniAtmosphere->CellCount())
	{
		return false;
	}
	std::vector<int> loadedParticleIndices;
	loadedParticleIndices.reserve(static_cast<size_t>(parsed.particlesCount));
	for (int index = 0; index < simulation.parts.active; ++index)
		if (simulation.parts[index].type)
			loadedParticleIndices.push_back(index);
	if (loadedParticleIndices.size() != static_cast<size_t>(parsed.particlesCount))
		return false;
	for (int index = 0; index < parsed.particlesCount; ++index)
	{
		const auto &expected = parsed.particles[index];
		const auto loadedIndex = loadedParticleIndices[static_cast<size_t>(index)];
		const auto &actual = simulation.parts[loadedIndex];
		if (expected.type != actual.type || expected.life != actual.life ||
			expected.ctype != actual.ctype || expected.tmp != actual.tmp ||
			expected.tmp2 != actual.tmp2 || expected.tmp3 != actual.tmp3 ||
			expected.tmp4 != actual.tmp4 || expected.flags != actual.flags ||
			expected.dcolour != actual.dcolour ||
			!PortableNearlyEqual(expected.x, actual.x, 0.500001, 0.0) ||
			!PortableNearlyEqual(expected.y, actual.y, 0.500001, 0.0) ||
			!PortableNearlyEqual(expected.vx, actual.vx, (1.0 / 16.0) + 1.0e-6, 0.0) ||
			!PortableNearlyEqual(expected.vy, actual.vy, (1.0 / 16.0) + 1.0e-6, 0.0) ||
			!PortableNearlyEqual(expected.temp, actual.temp, 0.500001, 0.0))
		{
			return false;
		}
		if (parsed.hasOmniWaterParcelState &&
			(!PortableNearlyEqual(parsed.omniWaterParcelMassKg[index],
				simulation.GetOmniWaterParcelMassKg(loadedIndex), 1.0e-15, 1.0e-9) ||
			 !PortableNearlyEqual(parsed.omniWaterParcelSpecificEnthalpyJPerKg[index],
				simulation.GetOmniWaterParcelSpecificEnthalpyJPerKg(loadedIndex), 1.0e-9, 1.0e-9)))
		{
			return false;
		}
		if (parsed.hasOmniCarbonParcelState &&
			!PortableNearlyEqual(parsed.omniCarbonParcelMassKg[index],
				simulation.GetOmniCarbonParcelMassKg(loadedIndex), 1.0e-15, 1.0e-9))
		{
			return false;
		}
		if (parsed.hasOmniSolutionState &&
			(!PortableNearlyEqual(parsed.omniSolutionSolventMassKg[index],
				simulation.GetOmniSolutionSolventMassKg(loadedIndex), 1.0e-15, 1.0e-9) ||
			 !PortableNearlyEqual(parsed.omniSolutionSoluteMassKg[index],
				simulation.GetOmniSolutionSoluteMassKg(loadedIndex), 1.0e-15, 1.0e-9) ||
			 !PortableNearlyEqual(parsed.omniSolutionNeutralSaltMassKg[index],
				simulation.GetOmniSolutionNeutralSaltMassKg(loadedIndex), 1.0e-15, 1.0e-9)))
		{
			return false;
		}
		if (parsed.hasOmniCorrosionState &&
			(!PortableNearlyEqual(parsed.omniCorrosionProgress[index],
				simulation.GetOmniCorrosionProgress(loadedIndex), 1.0e-12, 1.0e-9) ||
			 !PortableNearlyEqual(parsed.omniCorrosionPassivation[index],
				simulation.GetOmniCorrosionPassivation(loadedIndex), 1.0e-12, 1.0e-9)))
		{
			return false;
		}
	}
	const auto speciesCount = simulation.omniAtmosphere->SpeciesCount();
	for (size_t cell = 0; cell < simulation.omniAtmosphere->CellCount(); ++cell)
	{
		if (!parsed.omniAtmosphereCellValid[cell])
			return false;
		const auto x = cell % simulation.omniAtmosphere->Width();
		const auto y = cell / simulation.omniAtmosphere->Width();
		for (size_t species = 0; species < speciesCount; ++species)
		{
			const auto expected = parsed.omniAtmosphereSpeciesMassDensity[cell * speciesCount + species];
			if (!PortableNearlyEqual(expected,
				simulation.omniAtmosphere->SpeciesMassDensity(x, y, species), 1.0e-12, 1.0e-9))
			{
				return false;
			}
		}
		const auto &state = simulation.omniAtmosphere->State(x, y);
		const auto primitive = simulation.omniAtmosphere->Primitive(x, y);
		if (!primitive.finite ||
			!PortableNearlyEqual(parsed.omniAtmosphereMomentumX[cell], state.momentumX, 1.0e-12, 1.0e-9) ||
			!PortableNearlyEqual(parsed.omniAtmosphereMomentumY[cell], state.momentumY, 1.0e-12, 1.0e-9) ||
			!PortableNearlyEqual(parsed.omniAtmosphereTotalEnergy[cell], state.totalEnergy, 1.0e-9, 1.0e-9) ||
			!PortableNearlyEqual(parsed.omniAtmosphereCondensedWaterDensity[cell],
				primitive.condensedWaterDensity, 1.0e-12, 1.0e-9))
		{
			return false;
		}
	}
	return PortableParticleStateFinite(simulation) && PortableFixtureInventoryPresent(simulation);
}

void PortableRuntimeStep(Simulation &simulation)
{
	simulation.BeforeSim(true);
	simulation.UpdateParticles(0, simulation.parts.active);
	simulation.AfterSim();
}

void WritePortableRuntimeJson(const char *path, const PortableRuntimeEvidence &evidence,
	const char *reason)
{
	if (!path || !*path)
		return;
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	if (!output)
		return;
	const auto passed = evidence.Passed();
	const auto saveReloadPassed = evidence.SaveReloadPassed();
	const auto stateValidatePassed = evidence.StateValidatePassed();
	output << "{\n"
		<< "  \"schema\": \"omnipack-release-evidence\",\n"
		<< "  \"schema_version\": 1,\n"
		<< "  \"payload_schema_version\": 2,\n"
		<< "  \"test\": \"portable_runtime\",\n"
		<< "  \"run_id\": \"" << evidence.runId << "\",\n"
		<< "  \"candidate_sha256\": \"" << evidence.candidateSha256 << "\",\n"
		<< "  \"status\": \"" << (passed ? "PASS" : "FAIL") << "\",\n"
		<< "  \"passed\": " << (passed ? "true" : "false") << ",\n"
		<< "  \"reason\": \"" << reason << "\",\n"
		<< "  \"launch_passed\": " << (evidence.launchPassed ? "true" : "false") << ",\n"
		<< "  \"fixture_created\": " << (evidence.fixtureCreated ? "true" : "false") << ",\n"
		<< "  \"initial_simulate_passed\": " << (evidence.initialSimulatePassed ? "true" : "false") << ",\n"
		<< "  \"initial_save_passed\": " << (evidence.initialSavePassed ? "true" : "false") << ",\n"
		<< "  \"initial_parse_passed\": " << (evidence.initialParsePassed ? "true" : "false") << ",\n"
		<< "  \"initial_reload_passed\": " << (evidence.initialReloadPassed ? "true" : "false") << ",\n"
		<< "  \"initial_state_validate_passed\": " << (evidence.initialStateValidatePassed ? "true" : "false") << ",\n"
		<< "  \"post_step_simulate_passed\": " << (evidence.postStepSimulatePassed ? "true" : "false") << ",\n"
		<< "  \"post_step_finite_passed\": " << (evidence.postStepFinitePassed ? "true" : "false") << ",\n"
		<< "  \"post_step_inventory_passed\": " << (evidence.postStepInventoryPassed ? "true" : "false") << ",\n"
		<< "  \"post_step_atmosphere_passed\": " << (evidence.postStepAtmospherePassed ? "true" : "false") << ",\n"
		<< "  \"post_step_save_passed\": " << (evidence.postStepSavePassed ? "true" : "false") << ",\n"
		<< "  \"post_step_parse_passed\": " << (evidence.postStepParsePassed ? "true" : "false") << ",\n"
		<< "  \"post_step_reload_passed\": " << (evidence.postStepReloadPassed ? "true" : "false") << ",\n"
		<< "  \"post_step_roundtrip_passed\": " << (evidence.postStepRoundtripPassed ? "true" : "false") << ",\n"
		<< "  \"save_reload_passed\": " << (saveReloadPassed ? "true" : "false") << ",\n"
		<< "  \"state_validate_passed\": " << (stateValidatePassed ? "true" : "false") << ",\n"
		<< "  \"enhanced_mode\": " << (evidence.enhancedMode ? "true" : "false") << ",\n"
		<< "  \"omni_state_present\": " << (evidence.omniStatePresent ? "true" : "false") << ",\n"
		<< "  \"water_sidecar_roundtrip\": " << (evidence.waterSidecarRoundtrip ? "true" : "false") << ",\n"
		<< "  \"runtime_completion_reached\": " << (evidence.runtimeCompletionReached ? "true" : "false") << ",\n"
		<< "  \"initial_particle_count\": " << evidence.initialParticleCount << ",\n"
		<< "  \"post_step_particle_count\": " << evidence.postStepParticleCount << ",\n"
		<< "  \"final_particle_count\": " << evidence.finalParticleCount << ",\n"
		<< "  \"atmosphere_non_finite_cells\": " << evidence.atmosphereNonFiniteCells << "\n"
		<< "}\n";
}

int RunPortableRuntimeValidation(const char *jsonPath, const char *runId,
	const char *candidateSha256)
{
	PortableRuntimeEvidence evidence;
	evidence.runId = runId ? runId : "";
	evidence.candidateSha256 = candidateSha256 ? candidateSha256 : "";
	try
	{
		if (evidence.runId.empty() || evidence.candidateSha256.size() != 64 ||
			!std::all_of(evidence.candidateSha256.begin(), evidence.candidateSha256.end(), [](char value) {
				return (value >= '0' && value <= '9') || (value >= 'A' && value <= 'F');
			}))
		{
			throw std::runtime_error("runtime_identity_invalid");
		}
		// This validation path runs before the normal ExplicitSingletons setup.
		// SimulationData construction reaches material initializers which consult
		// GlobalPrefs, so provide the same dependency explicitly in headless mode.
		GlobalPrefs globalPrefs;
		SimulationData simulationData;
		evidence.launchPassed = true;
		auto simulation = Simulation::Factory();
		simulation->SetOmniSimulationMode(OMNI_ENHANCED);
		const int dust = simulation->create_part(-1, 32, 32, PT_DUST);
		const int water = simulation->create_part(-1, 34, 32, PT_WATR);
		if (dust < 0 || water < 0)
			throw std::runtime_error("fixture_create_failed");
		evidence.fixtureCreated = true;
		simulation->parts[dust].temp = 421.0f;
		simulation->parts[water].temp = 300.0f;
		for (int step = 0; step < 8; ++step)
			PortableRuntimeStep(*simulation);
		evidence.initialSimulatePassed = true;
		long long initialNonFiniteCells = -1;
		if (!PortableParticleStateFinite(*simulation) ||
			!PortableAtmosphereStateFinite(*simulation, initialNonFiniteCells) ||
			!PortableFixtureInventoryPresent(*simulation))
		{
			throw std::runtime_error("initial_state_non_finite");
		}
		// Validate the raw post-step state above before applying the bounded
		// serialization-boundary repair used by the actual save operation.
		if (simulation->IsOmniAtmosphereActive() && simulation->omniAtmosphere &&
			!simulation->omniAtmosphere->EnsureSerializableState())
			throw std::runtime_error("initial_state_not_serializable");
		auto saved = simulation->Save(true, RES.OriginRect());
		if (!saved || !saved->hasOmniAtmosphereState || !saved->hasOmniWaterParcelState ||
			saved->omniSimulationMode != OMNI_ENHANCED ||
			saved->particlesCount != PortableLiveParticleCount(*simulation) ||
			!PortableGameSaveStateValid(*saved))
			throw std::runtime_error("save_failed");
		evidence.initialSavePassed = true;
		evidence.initialParticleCount = saved->particlesCount;
		auto serialised = saved->Serialise();
		if (serialised.first || serialised.second.empty())
			throw std::runtime_error("serialise_failed");
		GameSave parsed(serialised.second);
		if (!parsed.hasOmniAtmosphereState || !parsed.hasOmniWaterParcelState ||
			parsed.omniSimulationMode != OMNI_ENHANCED ||
			!PortableSerializedOmniStateMatches(*saved, parsed))
			throw std::runtime_error("parsed_omni_state_invalid");
		evidence.initialParsePassed = true;
		int savedDust = -1;
		int savedWater = -1;
		for (int index = 0; index < parsed.particlesCount; ++index)
		{
			if (parsed.particles[index].type == PT_DUST)
				savedDust = index;
			if (index < static_cast<int>(parsed.omniWaterParcelMassKg.size()) &&
				parsed.omniWaterParcelMassKg[index] > 0.0)
				savedWater = index;
		}
		if (savedDust < 0 || savedWater < 0 ||
			savedWater >= static_cast<int>(parsed.omniWaterParcelSpecificEnthalpyJPerKg.size()))
			throw std::runtime_error("saved_omni_fixture_missing");
		const auto expectedDust = parsed.particles[savedDust];
		const auto expectedWater = parsed.particles[savedWater];
		const double expectedWaterMass = parsed.omniWaterParcelMassKg[savedWater];
		const double expectedWaterEnthalpy = parsed.omniWaterParcelSpecificEnthalpyJPerKg[savedWater];
		auto reloaded = Simulation::Factory();
		reloaded->SetOmniSimulationMode(parsed.omniSimulationMode);
		reloaded->Load(&parsed, true, { 0, 0 });
		if (!reloaded->IsOmniAtmosphereActive() || !reloaded->CreateSnapshot() ||
			reloaded->parts.active <= std::max(savedDust, savedWater) ||
			!PortableLoadedOmniStateMatches(parsed, *reloaded))
			throw std::runtime_error("reload_state_invalid");
		evidence.initialReloadPassed = true;
		const auto &loadedDust = reloaded->parts[savedDust];
		const auto &loadedWater = reloaded->parts[savedWater];
		if (loadedDust.type != expectedDust.type || loadedDust.x != expectedDust.x ||
			loadedDust.y != expectedDust.y || !PortableNearlyEqual(loadedDust.temp, expectedDust.temp, 0.001, 0.0) ||
			loadedWater.type != expectedWater.type || loadedWater.x != expectedWater.x ||
			loadedWater.y != expectedWater.y || !PortableNearlyEqual(loadedWater.temp, expectedWater.temp, 0.001, 0.0) ||
			!PortableNearlyEqual(reloaded->GetOmniWaterParcelMassKg(savedWater), expectedWaterMass, 1.0e-15, 0.0) ||
			!PortableNearlyEqual(reloaded->GetOmniWaterParcelSpecificEnthalpyJPerKg(savedWater),
				expectedWaterEnthalpy, 1.0e-9, 0.0))
			throw std::runtime_error("reload_state_mismatch");
		evidence.initialStateValidatePassed = true;
		evidence.enhancedMode = true;
		evidence.omniStatePresent = true;
		PortableRuntimeStep(*reloaded);
		evidence.postStepSimulatePassed = true;
		evidence.postStepParticleCount = PortableLiveParticleCount(*reloaded);
		evidence.postStepFinitePassed = PortableParticleStateFinite(*reloaded);
	evidence.postStepInventoryPassed =
		evidence.postStepParticleCount == evidence.initialParticleCount &&
		PortableFixtureInventoryPresent(*reloaded);
		evidence.postStepAtmospherePassed =
			PortableAtmosphereStateFinite(*reloaded, evidence.atmosphereNonFiniteCells);
		if (!evidence.postStepFinitePassed || !evidence.postStepInventoryPassed ||
			!evidence.postStepAtmospherePassed)
		{
			throw std::runtime_error("post_step_state_invalid");
		}
		if (reloaded->IsOmniAtmosphereActive() && reloaded->omniAtmosphere &&
			!reloaded->omniAtmosphere->EnsureSerializableState())
			throw std::runtime_error("post_step_state_not_serializable");
		auto postStepSaved = reloaded->Save(true, RES.OriginRect());
		if (!postStepSaved || postStepSaved->particlesCount != evidence.postStepParticleCount ||
			!PortableGameSaveStateValid(*postStepSaved))
		{
			throw std::runtime_error("post_step_save_failed");
		}
		evidence.postStepSavePassed = true;
		auto postStepSerialised = postStepSaved->Serialise();
		if (postStepSerialised.first || postStepSerialised.second.empty())
			throw std::runtime_error("post_step_serialise_failed");
		GameSave postStepParsed(postStepSerialised.second);
		if (!PortableSerializedOmniStateMatches(*postStepSaved, postStepParsed))
			throw std::runtime_error("post_step_parse_failed");
		evidence.postStepParsePassed = true;
		auto finalReloaded = Simulation::Factory();
		finalReloaded->SetOmniSimulationMode(postStepParsed.omniSimulationMode);
		finalReloaded->Load(&postStepParsed, true, { 0, 0 });
		evidence.finalParticleCount = PortableLiveParticleCount(*finalReloaded);
		long long finalNonFiniteCells = -1;
		if (!PortableLoadedOmniStateMatches(postStepParsed, *finalReloaded) ||
			!PortableFixtureInventoryPresent(*finalReloaded) ||
			!PortableAtmosphereStateFinite(*finalReloaded, finalNonFiniteCells) ||
			evidence.finalParticleCount != evidence.postStepParticleCount)
		{
			throw std::runtime_error("post_step_reload_failed");
		}
		evidence.postStepReloadPassed = true;
		evidence.postStepRoundtripPassed = true;
		evidence.waterSidecarRoundtrip = true;
		evidence.runtimeCompletionReached = true;
		WritePortableRuntimeJson(jsonPath, evidence, "load_simulate_save_reload_post_step_roundtrip_passed");
		return 0;
	}
	catch (const std::exception &error)
	{
		WritePortableRuntimeJson(jsonPath, evidence, error.what());
		std::cerr << "portable-runtime-validation: FAIL " << error.what() << '\n';
		return 1;
	}
}

void PrintStartupDiagnostics()
{
	std::cout << APPNAME << ' ' << RELEASE_LABEL << '\n';
	std::cout << "Git commit: " << (VCS_TAG[0] ? VCS_TAG : "unknown") << '\n';
	std::cout << "Build type: " << (DEBUG ? "Debug" : "Release") << '\n';
	std::cout << "SDL: " << SDL_MAJOR_VERSION << '.' << SDL_MINOR_VERSION << '.'
#if TPT_SDL3
		<< SDL_MICRO_VERSION
#else
		<< SDL_PATCHLEVEL
#endif
		<< '\n';
	std::cout << "OmniCore: enabled\nAtmosphere: enabled\nChemistry: enabled\n";
	const auto status = OmniCompute::GetStatus();
	std::cout << "Compute backend: " << OmniCompute::BackendName(status.backend)
		<< " (" << status.detail << ")\n";
}
}

void LoadWindowPosition()
{
	if (Client::Ref().IsFirstRun())
	{
		return;
	}

	auto &prefs = GlobalPrefs::Ref();
	int savedWindowX = prefs.Get("WindowX", INT_MAX);
	int savedWindowY = prefs.Get("WindowY", INT_MAX);

	int borderTop, borderLeft;
	SDL_GetWindowBordersSize(sdl_window, &borderTop, &borderLeft, nullptr, nullptr);
	// Sometimes (Windows), the border size may not be reported for 200+ frames
	// So just have a default of 5 to ensure the window doesn't get stuck where it can't be moved
	if (borderTop == 0)
		borderTop = 5;

	SDL_Rect displayBounds;
	bool ok = false;
#if TPT_SDL3
	int numDisplays = 0;
	auto *displays = SDL_GetDisplays(&numDisplays);
#else
	int numDisplays = SDL_GetNumVideoDisplays();
#endif
	for (int i = 0; i < numDisplays; i++)
	{
#if TPT_SDL3
		SDL_GetDisplayBounds(displays[i], &displayBounds);
#else
		SDL_GetDisplayBounds(i, &displayBounds);
#endif
		if (savedWindowX + borderTop > displayBounds.x && savedWindowY + borderLeft > displayBounds.y &&
				savedWindowX + borderTop < displayBounds.x + displayBounds.w &&
				savedWindowY + borderLeft < displayBounds.y + displayBounds.h)
		{
			ok = true;
			break;
		}
	}
#if TPT_SDL3
	SDL_free(displays);
#endif
	if (ok)
		SDL_SetWindowPosition(sdl_window, savedWindowX + borderLeft, savedWindowY + borderTop);
}

void SaveWindowPosition()
{
	int x, y;
	SDL_GetWindowPosition(sdl_window, &x, &y);

	int borderTop, borderLeft;
	SDL_GetWindowBordersSize(sdl_window, &borderTop, &borderLeft, nullptr, nullptr);

	auto &prefs = GlobalPrefs::Ref();
	prefs.Set("WindowX", x - borderLeft);
	prefs.Set("WindowY", y - borderTop);
}

void LargeScreenDialog()
{
	StringBuilder message;
	auto scale = ui::Engine::Ref().windowFrameOps.scale;
	message << Localization::Ref().Tr("main.large_screen_switch") << scale << Localization::Ref().Tr("main.large_screen_mode");
	message << desktopWidth << "x" << desktopHeight << Localization::Ref().Tr("main.large_screen_detected") << WINDOWW * scale << "x" << WINDOWH * scale << Localization::Ref().Tr("main.large_screen_required");
	message << Localization::Ref().Tr("main.large_screen_undo");
	new ConfirmPrompt(Localization::Ref().Tr("main.large_screen_title"), message.Build(), { nullptr, []() {
		GlobalPrefs::Ref().Set("Scale", 1);
		ui::Engine::Ref().windowFrameOps.scale = 1;
	} });
}

void TickClient()
{
	Client::Ref().Tick();
}

static void BlueScreen(String detailMessage, std::optional<std::vector<String>> stackTrace)
{
	auto &engine = ui::Engine::Ref();
	engine.g->BlendFilledRect(engine.g->Size().OriginRect(), 0x1172A9_rgb .WithAlpha(0xD2));

	auto crashPrevLogPath = ByteString("crash.prev.log");
	auto crashLogPath = ByteString("crash.log");
	Platform::RenameFile(crashLogPath, crashPrevLogPath, true);

	StringBuilder crashInfo;
	crashInfo << "ERROR - Details: " << detailMessage << "\n";
	crashInfo << "An unrecoverable fault has occurred, please report it by visiting the website below\n\n  " << SERVER << "\n\n";
	crashInfo << "An attempt will be made to save all of this information to " << crashLogPath.FromUtf8() << " in your data folder.\n";
	crashInfo << "Please attach this file to your report.\n\n";
	crashInfo << "Version: " << VersionInfo().FromUtf8() << "\n";
	crashInfo << "Tag: " << VCS_TAG << "\n";
	crashInfo << "Date: " << format::UnixtimeToDate(time(nullptr), "%Y-%m-%dT%H:%M:%SZ", false).FromUtf8() << "\n";
	if (stackTrace)
	{
		crashInfo << "Stack trace; Main is at 0x" << Format::Hex() << intptr_t(Main) << ":\n";
		for (auto &item : *stackTrace)
		{
			crashInfo << " - " << item << "\n";
		}
	}
	else
	{
		crashInfo << "Stack trace not available\n";
	}
	String errorText = crashInfo.Build();
	constexpr auto width = 440;
	ui::TextWrapper tw;
	tw.Update(errorText, true, width);
	engine.g->BlendText(ui::Point((engine.g->Size().X - width) / 2, 80), tw.WrappedText(), 0xFFFFFF_rgb .WithAlpha(0xFF));

	auto crashLogData = errorText.ToUtf8();
	std::cerr << crashLogData << std::endl;
	Platform::WriteFile(crashLogData, crashLogPath);

	//Death loop
	SDL_Event event;
	auto running = true;
	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				running = false;
			}
		}
		blit(engine.g->Data());
	}

	// Don't use Platform::Exit, we're practically zombies at this point anyway.
#if defined(__MINGW32__) || defined(__APPLE__) || defined(__EMSCRIPTEN__) || defined(__OpenBSD__)
	// Come on...
	exit(-1);
#else
	quick_exit(-1);
#endif
}

static struct
{
	int sig;
	const char *message;
} signalMessages[] = {
	{ SIGSEGV, "Memory read/write error" },
	{ SIGFPE, "Floating point exception" },
	{ SIGILL, "Program execution exception" },
	{ SIGABRT, "Unexpected program abort" },
	{ 0, nullptr },
};

static void SigHandler(int signal)
{
	const char *message = "Unknown signal";
	for (auto *msg = signalMessages; msg->message; ++msg)
	{
		if (msg->sig == signal)
		{
			message = msg->message;
			break;
		}
	}
	BlueScreen(ByteString(message).FromUtf8(), Platform::StackTrace());
}

static void TerminateHandler()
{
	ByteString err = "std::terminate called without a current exception";
	auto eptr = std::current_exception();
	try
	{
		if (eptr)
		{
			std::rethrow_exception(eptr);
		}
	}
	catch (const std::exception &e)
	{
		err = "unhandled exception: " + ByteString(e.what());
	}
	catch (...)
	{
		err = "unhandled exception not derived from std::exception, cannot determine reason";
	}
	BlueScreen(err.FromUtf8(), Platform::StackTrace());
}

constexpr int SCALE_MAXIMUM = 10;
constexpr int SCALE_MARGIN = 30;

int GuessBestScale()
{
	const int widthNoMargin = desktopWidth - SCALE_MARGIN;
	const int widthGuess = widthNoMargin / WINDOWW;

	const int heightNoMargin = desktopHeight - SCALE_MARGIN;
	const int heightGuess = heightNoMargin / WINDOWH;

	int guess = std::min(widthGuess, heightGuess);
	if(guess < 1 || guess > SCALE_MAXIMUM)
		guess = 1;

	return guess;
}

struct ExplicitSingletons
{
	// These need to be listed in the order they are populated in main.
	std::unique_ptr<GlobalPrefs> globalPrefs;
	http::RequestManagerPtr requestManager;
	std::unique_ptr<Client> client;
	std::unique_ptr<SaveRenderer> saveRenderer;
	std::unique_ptr<Favorite> favorite;
	std::unique_ptr<ui::Engine> engine;
	std::unique_ptr<SimulationData> simulationData;
	std::unique_ptr<GameController> gameController;
};
static std::unique_ptr<ExplicitSingletons> explicitSingletons;

int main(int argc, char *argv[])
{
	Platform::SetupCrt();
	return Platform::InvokeMain(argc, argv);
}

int Main(int argc, char *argv[])
{
	// Keep the GPU proof opt-in and before the normal singleton/window setup.
	// This makes the diagnostic safe on machines without a GPU backend and
	// guarantees that a fallback probe cannot touch user preferences or saves.
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--gpu-probe") == 0 || std::strcmp(argv[i], "gpu-probe") == 0)
			return Platform::RunSDLGPUProbe();
		if (std::strcmp(argv[i], "--gpu-validate") == 0 || std::strcmp(argv[i], "gpu-validate") == 0)
		{
			const char *jsonPath = nullptr;
			bool forceFailure = false;
			for (int arg = i + 1; arg < argc; ++arg)
			{
				if (std::strcmp(argv[arg], "--force-gpu-init-failure") == 0)
					forceFailure = true;
				else if (std::strncmp(argv[arg], "--gpu-validate-json=", 20) == 0)
					jsonPath = argv[arg] + 20;
				else if (std::strcmp(argv[arg], "--gpu-validate-json") == 0 && arg + 1 < argc)
					jsonPath = argv[++arg];
			}
			return Platform::RunSDLGPUValidation(jsonPath, forceFailure);
		}
		if (std::strcmp(argv[i], "--cpu-fallback-validate") == 0)
		{
			const char *jsonPath = nullptr;
			for (int arg = i + 1; arg < argc; ++arg)
			{
				if (std::strncmp(argv[arg], "--cpu-fallback-json=", 20) == 0)
					jsonPath = argv[arg] + 20;
				else if (std::strcmp(argv[arg], "--cpu-fallback-json") == 0 && arg + 1 < argc)
					jsonPath = argv[++arg];
			}
			return Platform::RunCPUFallbackValidation(jsonPath);
		}
		if (std::strcmp(argv[i], "--portable-runtime-validate") == 0)
		{
			const char *jsonPath = nullptr;
			const char *runId = nullptr;
			const char *candidateSha256 = nullptr;
			for (int arg = i + 1; arg < argc; ++arg)
			{
				if (std::strncmp(argv[arg], "--portable-runtime-json=", 24) == 0)
					jsonPath = argv[arg] + 24;
				else if (std::strcmp(argv[arg], "--portable-runtime-json") == 0 && arg + 1 < argc)
					jsonPath = argv[++arg];
				else if (std::strncmp(argv[arg], "--portable-runtime-run-id=", 26) == 0)
					runId = argv[arg] + 26;
				else if (std::strcmp(argv[arg], "--portable-runtime-run-id") == 0 && arg + 1 < argc)
					runId = argv[++arg];
				else if (std::strncmp(argv[arg], "--portable-runtime-candidate-sha256=", 36) == 0)
					candidateSha256 = argv[arg] + 36;
				else if (std::strcmp(argv[arg], "--portable-runtime-candidate-sha256") == 0 && arg + 1 < argc)
					candidateSha256 = argv[++arg];
			}
			return RunPortableRuntimeValidation(jsonPath, runId, candidateSha256);
		}
		if (std::strcmp(argv[i], "--gui-smoke-test") == 0)
		{
			const char *jsonPath = nullptr;
			for (int arg = i + 1; arg < argc; ++arg)
			{
				if (std::strncmp(argv[arg], "--gui-smoke-json=", 17) == 0)
					jsonPath = argv[arg] + 17;
				else if (std::strcmp(argv[arg], "--gui-smoke-json") == 0 && arg + 1 < argc)
					jsonPath = argv[++arg];
			}
			return Platform::RunSDL3GUISmokeTest(jsonPath);
		}
	}

	Platform::Atexit([]() {
		Platform::ShutdownOmniCompute();
		SaveWindowPosition();
		// Unregister dodgy error handlers so they don't try to show the blue screen when the window is closed
		for (auto *msg = signalMessages; msg->message; ++msg)
		{
			signal(msg->sig, SIG_DFL);
		}
		SDLClose();
		explicitSingletons.reset();
	});
	explicitSingletons = std::make_unique<ExplicitSingletons>();


	// https://bugzilla.libsdl.org/show_bug.cgi?id=3796
#if TPT_SDL3
	if (!SDL_Init(0))
#else
	if (SDL_Init(0) < 0)
#endif
	{
		fprintf(stderr, "Initializing SDL: %s\n", SDL_GetError());
		return 1;
	}

	Platform::originalCwd = Platform::GetCwd();

	using Argument = std::optional<ByteString>;
	std::map<ByteString, Argument> arguments;

	for (auto i = 1; i < argc; ++i)
	{
		auto str = ByteString(argv[i]);
		if (str.BeginsWith("file://"))
		{
			arguments.insert({ "open", format::URLDecode(str.substr(7 /* length of the "file://" prefix */)) });
		}
		else if ((str.EndsWith(".cps") || str.EndsWith(".stm")) && Platform::FileExists(str))
		{
			arguments.insert({ "open", str });
		}
		else if (str.BeginsWith("ptsave:"))
		{
			arguments.insert({ "ptsave", str });
		}
		else if (auto split = str.SplitBy(':'))
		{
			arguments.insert({ split.Before(), split.After() });
		}
		else if (auto split = str.SplitBy('='))
		{
			arguments.insert({ split.Before(), split.After() });
		}
		else if (str == "open" || str == "ptsave" || str == "ddir")
		{
			if (i + 1 < argc)
			{
				arguments.insert({ str, argv[i + 1] });
				i += 1;
			}
			else
			{
				std::cerr << "no value provided for command line parameter " << str << std::endl;
			}
		}
		else
		{
			arguments.insert({ str, "" }); // so .has_value() is true
		}
	}

	auto ddirArg = arguments["ddir"];
	if (ddirArg.has_value())
	{
		if (Platform::ChangeDir(ddirArg.value()))
			Platform::sharedCwd = Platform::GetCwd();
		else
			perror("failed to chdir to requested ddir");
	}
	else if constexpr (SHARED_DATA_FOLDER)
	{
		auto ddir = Platform::DefaultDdir();
		if (!Platform::FileExists("powder.pref"))
		{
			if (ddir.size())
			{
				if (!Platform::ChangeDir(ddir))
				{
					perror("failed to chdir to default ddir");
					ddir = {};
				}
			}
		}

		if (ddir.size())
		{
			Platform::sharedCwd = ddir;
		}
	}
	// We're now in the correct directory, time to get prefs.
	explicitSingletons->globalPrefs = std::make_unique<GlobalPrefs>();

	auto &prefs = GlobalPrefs::Ref();

	// 初始化全局语言（在创建任何 UI 之前）
	// Fresh OmniPack installations start in Simplified Chinese. Existing users
	// keep their explicit preference and can switch languages in Options.
	int languageIndex = prefs.Get("Language", 1);
	Localization::Ref().SetLanguageIndex(languageIndex);

	WindowFrameOps windowFrameOps{
		prefs.Get("Scale", 1),
		prefs.Get("Resizable", false),
		prefs.Get("Fullscreen", false),
		prefs.Get("AltFullscreen", false),
		prefs.Get("ForceIntegerScaling", true),
		prefs.Get("BlurryScaling", false),
	};
	auto graveExitsConsole = prefs.Get("GraveExitsConsole", true);
	momentumScroll = prefs.Get("MomentumScroll", true);
	showAvatars = prefs.Get("ShowAvatars", true);

	auto trueString = [](ByteString str) {
		str = str.ToLower();
		return str == "true" ||
		       str == "t" ||
		       str == "on" ||
		       str == "yes" ||
		       str == "y" ||
		       str == "1" ||
		       str == ""; // standalone "redirect" or "disable-bluescreen" or similar arguments
	};
	auto trueArg = [&trueString](Argument arg) {
		return arg.has_value() && trueString(arg.value());
	};

	auto kioskArg = arguments["kiosk"];
	if (kioskArg.has_value())
	{
		windowFrameOps.fullscreen = trueString(kioskArg.value());
		prefs.Set("Fullscreen", windowFrameOps.fullscreen);
	}

	auto redirectStd = prefs.Get("RedirectStd", false);
	if (trueArg(arguments["console"]))
	{
		Platform::AllocConsole();
	}
	else if (trueArg(arguments["redirect"]) || redirectStd)
	{
		FILE *new_stdout = freopen("stdout.log", "w", stdout);
		FILE *new_stderr = freopen("stderr.log", "w", stderr);
		if (!new_stdout || !new_stderr)
		{
			Platform::Exit(42);
		}
	}

	auto scaleArg = arguments["scale"];
	if (scaleArg.has_value())
	{
		try
		{
			windowFrameOps.scale = scaleArg.value().ToNumber<int>();
			prefs.Set("Scale", windowFrameOps.scale);
		}
		catch (const std::runtime_error &e)
		{
			std::cerr << "failed to set scale: " << e.what() << std::endl;
		}
	}

	auto clientConfig = [&prefs](Argument arg, ByteString name) {
		if (!arg)
		{
			return prefs.Get<ByteString>(name);
		}
		if (arg->empty())
		{
			arg.reset();
		}
		prefs.Set(name, arg);
		return arg;
	};
	auto proxyString = clientConfig(arguments["proxy"], "Proxy");
	auto cafileString = clientConfig(arguments["cafile"], "CAFile");
	auto capathString = clientConfig(arguments["capath"], "CAPath");
	bool disableNetwork = trueArg(arguments["disable-network"]);
	explicitSingletons->requestManager = http::RequestManager::Create({ proxyString, cafileString, capathString, disableNetwork });

	explicitSingletons->client = std::make_unique<Client>();
	Client::Ref().SetAutoStartupRequest(prefs.Get("AutoStartupRequest", true));
	Client::Ref().Initialize();
	Client::Ref().SetRedirectStd(redirectStd);

	explicitSingletons->saveRenderer = std::make_unique<SaveRenderer>();
	explicitSingletons->favorite = std::make_unique<Favorite>();
	explicitSingletons->engine = std::make_unique<ui::Engine>();

	// TODO: maybe bind the maximum allowed scale to screen size somehow
	if(windowFrameOps.scale < 1 || windowFrameOps.scale > SCALE_MAXIMUM)
		windowFrameOps.scale = 1;

	auto &engine = ui::Engine::Ref();
	engine.g = new Graphics();
	engine.GraveExitsConsole = graveExitsConsole;
	engine.MomentumScroll = momentumScroll;
	engine.ShowAvatars = showAvatars;
	engine.Begin();
	engine.SetFastQuit(prefs.Get("FastQuit", true));
	engine.SetGlobalQuit(prefs.Get("GlobalQuit", true));
	engine.TouchUI = prefs.Get("TouchUI", DEFAULT_TOUCH_UI);
	engine.windowFrameOps = windowFrameOps;

	if (auto drawLimit = GlobalPrefs::Ref().Get<int>("DrawLimit"); drawLimit && *drawLimit == -1)
	{
		engine.SetDrawingFrequencyLimit(DrawLimitDisplay{});
	}
	else if (drawLimit && *drawLimit >= DrawLimitExplicit::minSane && *drawLimit < DrawLimitExplicit::maxSane)
	{
		engine.SetDrawingFrequencyLimit(DrawLimitExplicit{ *drawLimit });
	}
	else
	{
		engine.SetDrawingFrequencyLimit(DefaultDrawLimit);
	}

	SDLOpen();
	Platform::InitializeOmniCompute();
	PrintStartupDiagnostics();

	if (Client::Ref().IsFirstRun() && FORCE_WINDOW_FRAME_OPS == forceWindowFrameOpsNone)
	{
		auto guessed = GuessBestScale();
		if (engine.windowFrameOps.scale != guessed)
		{
			engine.windowFrameOps.scale = guessed;
			prefs.Set("Scale", guessed);
			showLargeScreenDialog = true;
		}
	}

	bool enableBluescreen = USE_BLUESCREEN && !trueArg(arguments["disable-bluescreen"]);
	if (enableBluescreen)
	{
		//Get ready to catch any dodgy errors
		for (auto *msg = signalMessages; msg->message; ++msg)
		{
			signal(msg->sig, SigHandler);
		}
		std::set_terminate(TerminateHandler);
	}

	if constexpr (X86_KILL_DENORMALS)
	{
		X86KillDenormals();
	}

	explicitSingletons->simulationData = std::make_unique<SimulationData>();
	explicitSingletons->gameController = std::make_unique<GameController>();
	auto *gameController = explicitSingletons->gameController.get();
	engine.ShowWindow(gameController->GetView());
	gameController->InitCommandInterface();

	auto openArg = arguments["open"];
	if (openArg.has_value())
	{
		if constexpr (DEBUG)
		{
			std::cout << "Loading " << openArg.value() << std::endl;
		}
		if (Platform::FileExists(openArg.value()))
		{
			try
			{
				std::vector<char> gameSaveData;
				if (!Platform::ReadFile(gameSaveData, openArg.value()))
				{
					new ErrorMessage(Localization::Ref().Tr("common.error"), Localization::Ref().Tr("main.error_could_not_read_file"));
				}
				else
				{
					auto newFile = std::make_unique<SaveFile>(openArg.value());
					auto newSave = std::make_unique<GameSave>(std::move(gameSaveData));
					newFile->SetGameSave(std::move(newSave));
					gameController->LoadSaveFile(std::move(newFile));
				}

			}
			catch (std::exception & e)
			{
				new ErrorMessage(Localization::Ref().Tr("common.error"), Localization::Ref().Tr("main.error_could_not_open_save_prefix") + ByteString(e.what()).FromUtf8()) ;
			}
		}
		else
		{
			new ErrorMessage(Localization::Ref().Tr("common.error"), Localization::Ref().Tr("main.error_could_not_open_file"));
		}
	}

	auto ptsaveArg = arguments["ptsave"];
	if (ptsaveArg.has_value())
	{
		engine.g->Clear();
		engine.g->DrawRect(RectSized(engine.g->Size() / 2 - Vec2(100, 25), Vec2(200, 50)), 0xB4B4B4_rgb);
		String loadingText = Localization::Ref().Tr("main.loading_save");
		engine.g->BlendText(engine.g->Size() / 2 - Vec2((Graphics::TextSize(loadingText).X - 1) / 2, 5), loadingText, style::Colour::InformationTitle);

		blit(engine.g->Data());
		try
		{
			ByteString saveIdPart;
			if (ByteString::Split split = ptsaveArg.value().SplitBy(':'))
			{
				if (split.Before() != "ptsave")
					throw std::runtime_error("Not a ptsave link");
				saveIdPart = split.After().SplitBy('#').Before();
			}
			else
				throw std::runtime_error("Invalid save link");

			if (!saveIdPart.size())
				throw std::runtime_error("No Save ID");
			if constexpr (DEBUG)
			{
				std::cout << "Got Ptsave: id: " << saveIdPart << std::endl;
			}
			ByteString saveHistoryPart = "0";
			if (auto split = saveIdPart.SplitBy('@'))
			{
				saveHistoryPart = split.After();
				saveIdPart = split.Before();
			}
			int saveId = saveIdPart.ToNumber<int>();
			int saveHistory = saveHistoryPart.ToNumber<int>();
			gameController->OpenSavePreview(saveId, saveHistory, savePreviewUrl);
		}
		catch (std::exception & e)
		{
			new ErrorMessage(Localization::Ref().Tr("common.error"), ByteString(e.what()).FromUtf8());
			Platform::MarkPresentable();
		}
	}
	else
	{
		Platform::MarkPresentable();
	}

	MainLoop();

	Platform::Exit(0);
	return 0;
}
