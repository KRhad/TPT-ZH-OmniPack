#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/Air.h"
#include "simulation/ElementClasses.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"

#include <fstream>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace
{
void Advance(Simulation &simulation)
{
	simulation.BeforeSim(true);
	simulation.UpdateParticles(0, simulation.parts.active);
	simulation.AfterSim();
}

bool NearlyEqual(double lhs, double rhs, double absoluteTolerance = 1.0e-6,
	double relativeTolerance = 1.0e-6)
{
	if (!std::isfinite(lhs) || !std::isfinite(rhs))
		return false;
	return std::abs(lhs - rhs) <= absoluteTolerance +
		relativeTolerance * std::max(std::abs(lhs), std::abs(rhs));
}

template <typename T, typename Compare>
bool EqualVector(const std::vector<T> &lhs, const std::vector<T> &rhs, Compare compare)
{
	if (lhs.size() != rhs.size())
		return false;
	for (size_t index = 0; index < lhs.size(); ++index)
		if (!compare(lhs[index], rhs[index]))
			return false;
	return true;
}

template <typename T, typename Compare>
bool EqualPlane(const PlaneAdapter<std::vector<T>> &lhs,
	const PlaneAdapter<std::vector<T>> &rhs, Compare compare)
{
	return lhs.Size() == rhs.Size() && EqualVector(lhs.Base, rhs.Base, compare);
}

bool EqualSignState(const sign &lhs, const sign &rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.ju == rhs.ju && lhs.text == rhs.text;
}

bool EqualStkmState(const StkmData &lhs, const StkmData &rhs)
{
	return lhs.rocketBoots1 == rhs.rocketBoots1 && lhs.rocketBoots2 == rhs.rocketBoots2 &&
		lhs.fan1 == rhs.fan1 && lhs.fan2 == rhs.fan2 &&
		lhs.rocketBootsFigh == rhs.rocketBootsFigh && lhs.fanFigh == rhs.fanFigh;
}

void ApplySaveSimulationParameters(Simulation &simulation, const GameSave &save)
{
	simulation.gravityMode = save.gravityMode;
	simulation.customGravityX = save.customGravityX;
	simulation.customGravityY = save.customGravityY;
	simulation.air->airMode = save.airMode;
	simulation.air->ambientAirTemp = save.ambientAirTemp;
	simulation.air->edgePressure = save.edgePressure;
	simulation.air->edgeVelocityX = save.edgeVelocityX;
	simulation.air->edgeVelocityY = save.edgeVelocityY;
	simulation.air->vorticityCoeff = save.vorticityCoeff;
	simulation.air->convectionMode = save.convectionMode;
	simulation.SetOmniSimulationMode(save.omniSimulationMode);
	simulation.SetEdgeMode(save.edgeMode);
	simulation.legacy_enable = save.legacyEnable;
	simulation.water_equal_test = save.waterEEnabled;
	simulation.aheat_enable = save.aheatEnable;
	simulation.EnableNewtonianGravity(save.gravityEnable);
	simulation.frameCount = save.frameCount;
	if (save.hasRngState)
		simulation.rng.state(save.rngState);
	else
		simulation.rng = RNG();
	simulation.ensureDeterminism = save.ensureDeterminism;
}

void LoadWholeSave(Simulation &simulation, const GameSave &save)
{
	// Match GameModel's real whole-save path: parameters are needed before
	// clear/load for Air/gravity construction, then deterministic continuation
	// state is restored after particle create callbacks have run.
	ApplySaveSimulationParameters(simulation, save);
	simulation.clear_sim();
	simulation.Load(&save, true, { 0, 0 });
	ApplySaveSimulationParameters(simulation, save);
}

template <typename Callback>
void RequireRejected(const char *name, Callback callback)
{
	try
	{
		callback();
	}
	catch (const std::exception &)
	{
		return;
	}
	throw std::runtime_error(std::string("negative state mutation was accepted: ") + name);
}

bool EqualParticleState(const Particle &lhs, const Particle &rhs)
{
	const auto expectedLife = std::clamp(lhs.life, 0, 0xFFFF);
	if (lhs.type != rhs.type || expectedLife != rhs.life || lhs.ctype != rhs.ctype ||
		lhs.tmp3 != rhs.tmp3 || lhs.tmp4 != rhs.tmp4 ||
		(lhs.type != PT_SOAP && lhs.tmp2 != rhs.tmp2))
		return false;
	const auto expectedDcolour = lhs.dcolour &&
		((lhs.dcolour & 0xFF000000U) || lhs.type == PT_LIFE) ? lhs.dcolour : 0U;
	if (expectedDcolour != rhs.dcolour)
		return false;
	// SOAP/FIGH use temporary fields as runtime links and the loader is
	// allowed to remap those links. All physical/state-bearing fields remain
	// part of the comparison.
	if (lhs.type != PT_SOAP && lhs.type != PT_FIGH && lhs.tmp != rhs.tmp)
		return false;
	return NearlyEqual(lhs.x, rhs.x, 0.500001, 0.0) &&
		NearlyEqual(lhs.y, rhs.y, 0.500001, 0.0) &&
		NearlyEqual(std::clamp(lhs.vx, -127.0f / 16.0f, 128.0f / 16.0f), rhs.vx,
			(1.0 / 16.0) + 1.0e-6, 0.0) &&
		NearlyEqual(std::clamp(lhs.vy, -127.0f / 16.0f, 128.0f / 16.0f), rhs.vy,
			(1.0 / 16.0) + 1.0e-6, 0.0) &&
		NearlyEqual(lhs.temp, rhs.temp, 0.500001, 0.0);
}

auto ParticleSortKey(const Particle &particle)
{
	const auto quantizePosition = [](float value) {
		return std::isfinite(value) ? static_cast<long long>(std::llround(value)) :
			std::numeric_limits<long long>::min();
	};
	const auto quantizeVelocity = [](float value) {
		return std::isfinite(value) ? static_cast<long long>(std::llround(
			std::clamp(value, -127.0f / 16.0f, 128.0f / 16.0f) * 16.0f)) :
			std::numeric_limits<long long>::min();
	};
	const auto quantizeTemperature = [](float value) {
		return std::isfinite(value) ? static_cast<long long>(std::floor(value + 0.5f)) :
			std::numeric_limits<long long>::min();
	};
	const auto canonicalDcolour = particle.dcolour &&
		((particle.dcolour & 0xFF000000U) || particle.type == PT_LIFE) ? particle.dcolour : 0U;
	const auto canonicalTmp = particle.type == PT_SOAP || particle.type == PT_FIGH ? 0 : particle.tmp;
	return std::tuple{
		particle.type, quantizePosition(particle.y), quantizePosition(particle.x), particle.ctype,
		std::clamp(particle.life, 0, 0xFFFF), canonicalTmp, particle.tmp2, particle.tmp3, particle.tmp4,
		canonicalDcolour, quantizeTemperature(particle.temp), quantizeVelocity(particle.vx),
		quantizeVelocity(particle.vy),
	};
}

struct LoadedParticle
{
	int index = -1;
	Particle particle{};
	std::array<double, 8> sidecars{};
};

struct SavedParticle
{
	size_t sourceIndex = 0;
	Particle particle{};
	std::array<double, 8> sidecars{};
};

std::array<double, 8> SavedSidecars(const GameSave &save, size_t index)
{
	std::array<double, 8> values{};
	if (save.hasOmniWaterParcelState)
	{
		values[0] = save.omniWaterParcelMassKg[index];
		values[1] = save.omniWaterParcelSpecificEnthalpyJPerKg[index];
	}
	if (save.hasOmniCarbonParcelState)
		values[2] = save.omniCarbonParcelMassKg[index];
	if (save.hasOmniSolutionState)
	{
		values[3] = save.omniSolutionSolventMassKg[index];
		values[4] = save.omniSolutionSoluteMassKg[index];
		values[5] = save.omniSolutionNeutralSaltMassKg[index];
	}
	if (save.hasOmniCorrosionState)
	{
		values[6] = save.omniCorrosionProgress[index];
		values[7] = save.omniCorrosionPassivation[index];
	}
	return values;
}

std::array<double, 8> LoadedSidecars(const Simulation &simulation, const GameSave &save,
	int index)
{
	std::array<double, 8> values{};
	if (save.hasOmniWaterParcelState)
	{
		values[0] = simulation.GetOmniWaterParcelMassKg(index);
		values[1] = simulation.GetOmniWaterParcelSpecificEnthalpyJPerKg(index);
	}
	if (save.hasOmniCarbonParcelState)
		values[2] = simulation.GetOmniCarbonParcelMassKg(index);
	if (save.hasOmniSolutionState)
	{
		values[3] = simulation.GetOmniSolutionSolventMassKg(index);
		values[4] = simulation.GetOmniSolutionSoluteMassKg(index);
		values[5] = simulation.GetOmniSolutionNeutralSaltMassKg(index);
	}
	if (save.hasOmniCorrosionState)
	{
		values[6] = simulation.GetOmniCorrosionProgress(index);
		values[7] = simulation.GetOmniCorrosionPassivation(index);
	}
	return values;
}

template <typename ParticleRecord>
bool ParticleRecordLess(const ParticleRecord &lhs, const ParticleRecord &rhs)
{
	const auto lhsParticle = ParticleSortKey(lhs.particle);
	const auto rhsParticle = ParticleSortKey(rhs.particle);
	if (lhsParticle != rhsParticle)
		return lhsParticle < rhsParticle;
	return lhs.sidecars < rhs.sidecars;
}

std::vector<SavedParticle> CollectSavedParticles(const GameSave &save)
{
	std::vector<SavedParticle> particles;
	particles.reserve(static_cast<size_t>(save.particlesCount));
	for (int index = 0; index < save.particlesCount; ++index)
		particles.push_back({ static_cast<size_t>(index), save.particles[index],
			SavedSidecars(save, static_cast<size_t>(index)) });
	std::sort(particles.begin(), particles.end(), ParticleRecordLess<SavedParticle>);
	return particles;
}

std::vector<LoadedParticle> CollectLoadedParticles(const Simulation &simulation,
	const GameSave &save)
{
	std::vector<LoadedParticle> particles;
	for (int index = 0; index < simulation.parts.active; ++index)
	{
		if (simulation.parts[index].type)
			particles.push_back({ index, simulation.parts[index],
				LoadedSidecars(simulation, save, index) });
	}
	std::sort(particles.begin(), particles.end(), ParticleRecordLess<LoadedParticle>);
	return particles;
}

void RequireSerializedLegacyStateEquality(const GameSave &saved, const GameSave &parsed)
{
	const bool expectedAmbientHeat = saved.hasAmbientHeat && saved.aheatEnable;
	const bool expectedGravityMaps = saved.hasGravityMaps && saved.gravityEnable;
	if (saved.hasPressure != parsed.hasPressure ||
		expectedAmbientHeat != parsed.hasAmbientHeat ||
		saved.hasBlockAirMaps != parsed.hasBlockAirMaps ||
		expectedGravityMaps != parsed.hasGravityMaps)
		throw std::runtime_error("serialized Legacy field metadata changed");

	const auto equalByte = [](auto lhs, auto rhs) { return lhs == rhs; };
	const auto equalGravityFloat = [](float lhs, float rhs) {
		return NearlyEqual(lhs, rhs, 1.0e-6, 1.0e-6);
	};
	const auto equalOpsField = [](float lhs, float rhs) {
		if (!std::isfinite(lhs) || !std::isfinite(rhs))
			return false;
		const auto canonical = std::clamp(lhs, -255.0f, 255.0f);
		return NearlyEqual(canonical, rhs, (1.0 / 128.0) + 1.0e-6, 0.0);
	};
	const auto equalAmbientHeat = [](float lhs, float rhs) {
		return std::isfinite(lhs) && std::isfinite(rhs) &&
			NearlyEqual(lhs, rhs, 0.500001, 0.0);
	};
	if (!EqualPlane(saved.blockMap, parsed.blockMap, equalByte))
		throw std::runtime_error("serialized wall map changed");
	if (saved.fanVelX.Size() != parsed.fanVelX.Size() ||
		saved.fanVelY.Size() != parsed.fanVelY.Size())
		throw std::runtime_error("serialized fan map dimensions changed");
	for (size_t index = 0; index < saved.blockMap.Base.size(); ++index)
	{
		if (saved.blockMap.Base[index] != WL_FAN)
			continue;
		const auto equalFan = [](float lhs, float rhs) {
			if (!std::isfinite(lhs) || !std::isfinite(rhs))
				return false;
			const auto canonical = std::clamp(lhs, -127.0f / 64.0f, 128.0f / 64.0f);
			return NearlyEqual(canonical, rhs, (1.0 / 64.0) + 1.0e-6, 0.0);
		};
		if (!equalFan(saved.fanVelX.Base[index], parsed.fanVelX.Base[index]) ||
			!equalFan(saved.fanVelY.Base[index], parsed.fanVelY.Base[index]))
			throw std::runtime_error("serialized fan velocity changed");
	}
	if (saved.hasPressure &&
		(!EqualPlane(saved.pressure, parsed.pressure, equalOpsField) ||
		 !EqualPlane(saved.velocityX, parsed.velocityX, equalOpsField) ||
		 !EqualPlane(saved.velocityY, parsed.velocityY, equalOpsField)))
		throw std::runtime_error("serialized pressure or air velocity changed");
	if (expectedAmbientHeat &&
		!EqualPlane(saved.ambientHeat, parsed.ambientHeat, equalAmbientHeat))
		throw std::runtime_error("serialized ambient heat changed");
	if (saved.hasBlockAirMaps &&
		(!EqualPlane(saved.blockAir, parsed.blockAir, equalByte) ||
		 !EqualPlane(saved.blockAirh, parsed.blockAirh, equalByte)))
		throw std::runtime_error("serialized block-air maps changed");
	if (expectedGravityMaps &&
		(!EqualPlane(saved.gravMass, parsed.gravMass, equalGravityFloat) ||
		 !EqualPlane(saved.gravMask, parsed.gravMask, equalByte) ||
		 !EqualPlane(saved.gravForceX, parsed.gravForceX, equalGravityFloat) ||
		 !EqualPlane(saved.gravForceY, parsed.gravForceY, equalGravityFloat)))
		throw std::runtime_error("serialized gravity maps changed");

	if (saved.waterEEnabled != parsed.waterEEnabled ||
		saved.legacyEnable != parsed.legacyEnable ||
		saved.gravityEnable != parsed.gravityEnable ||
		saved.aheatEnable != parsed.aheatEnable || saved.paused != parsed.paused ||
		saved.gravityMode != parsed.gravityMode || saved.airMode != parsed.airMode ||
		saved.convectionMode != parsed.convectionMode || saved.edgeMode != parsed.edgeMode)
		throw std::runtime_error("serialized simulation options changed");
	const auto expectedCustomGravityX = saved.gravityMode == GRAV_CUSTOM ? saved.customGravityX : 0.0f;
	const auto expectedCustomGravityY = saved.gravityMode == GRAV_CUSTOM ? saved.customGravityY : 0.0f;
	const auto expectedEdgePressure = std::abs(saved.edgePressure) > 0.0001f ? saved.edgePressure : 0.0f;
	const auto expectedEdgeVelocityX = std::abs(saved.edgeVelocityX) > 0.0001f ? saved.edgeVelocityX : 0.0f;
	const auto expectedEdgeVelocityY = std::abs(saved.edgeVelocityY) > 0.0001f ? saved.edgeVelocityY : 0.0f;
	const auto expectedVorticity = saved.vorticityCoeff > 0.0001f && saved.vorticityCoeff < 1.0f
		? saved.vorticityCoeff : 0.0f;
	if (!NearlyEqual(expectedCustomGravityX, parsed.customGravityX, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(expectedCustomGravityY, parsed.customGravityY, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(saved.ambientAirTemp, parsed.ambientAirTemp, 1.0e-4, 1.0e-7) ||
		!NearlyEqual(expectedEdgePressure, parsed.edgePressure, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(expectedEdgeVelocityX, parsed.edgeVelocityX, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(expectedEdgeVelocityY, parsed.edgeVelocityY, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(expectedVorticity, parsed.vorticityCoeff, 1.0e-6, 1.0e-6))
		throw std::runtime_error("serialized floating-point simulation options changed");

	if (saved.ensureDeterminism != parsed.ensureDeterminism ||
		parsed.hasRngState != saved.ensureDeterminism)
		throw std::runtime_error("serialized deterministic-state metadata changed");
	if (saved.ensureDeterminism &&
		(saved.frameCount != parsed.frameCount || saved.rngState != parsed.rngState))
		throw std::runtime_error("serialized frame or RNG state changed");
	if (!EqualVector(saved.signs, parsed.signs, EqualSignState) ||
		!EqualStkmState(saved.stkm, parsed.stkm))
		throw std::runtime_error("serialized sign or stickman state changed");
}

void RequireSerializedStateEquality(const GameSave &saved, const GameSave &parsed)
{
	if (saved.blockSize != parsed.blockSize || saved.particlesCount != parsed.particlesCount ||
		saved.particles.size() < static_cast<size_t>(saved.particlesCount) ||
		parsed.particles.size() < static_cast<size_t>(parsed.particlesCount))
		throw std::runtime_error("serialized particle inventory changed");
	const auto requireSidecarSize = [&](bool enabled, const auto &values, const char *name) {
		if (enabled && values.size() != static_cast<size_t>(saved.particlesCount))
			throw std::runtime_error(std::string("malformed serialized sidecar: ") + name);
	};
	requireSidecarSize(saved.hasOmniWaterParcelState, saved.omniWaterParcelMassKg, "water_mass");
	requireSidecarSize(saved.hasOmniWaterParcelState, saved.omniWaterParcelSpecificEnthalpyJPerKg, "water_enthalpy");
	requireSidecarSize(saved.hasOmniCarbonParcelState, saved.omniCarbonParcelMassKg, "carbon_mass");
	requireSidecarSize(saved.hasOmniSolutionState, saved.omniSolutionSolventMassKg, "solution_solvent");
	requireSidecarSize(saved.hasOmniSolutionState, saved.omniSolutionSoluteMassKg, "solution_solute");
	requireSidecarSize(saved.hasOmniSolutionState, saved.omniSolutionNeutralSaltMassKg, "solution_salt");
	requireSidecarSize(saved.hasOmniCorrosionState, saved.omniCorrosionProgress, "corrosion_progress");
	requireSidecarSize(saved.hasOmniCorrosionState, saved.omniCorrosionPassivation, "corrosion_passivation");
	requireSidecarSize(saved.hasOmniWaterParcelState, parsed.omniWaterParcelMassKg, "parsed_water_mass");
	requireSidecarSize(saved.hasOmniWaterParcelState, parsed.omniWaterParcelSpecificEnthalpyJPerKg, "parsed_water_enthalpy");
	requireSidecarSize(saved.hasOmniCarbonParcelState, parsed.omniCarbonParcelMassKg, "parsed_carbon_mass");
	requireSidecarSize(saved.hasOmniSolutionState, parsed.omniSolutionSolventMassKg, "parsed_solution_solvent");
	requireSidecarSize(saved.hasOmniSolutionState, parsed.omniSolutionSoluteMassKg, "parsed_solution_solute");
	requireSidecarSize(saved.hasOmniSolutionState, parsed.omniSolutionNeutralSaltMassKg, "parsed_solution_salt");
	requireSidecarSize(saved.hasOmniCorrosionState, parsed.omniCorrosionProgress, "parsed_corrosion_progress");
	requireSidecarSize(saved.hasOmniCorrosionState, parsed.omniCorrosionPassivation, "parsed_corrosion_passivation");
	const auto savedParticles = CollectSavedParticles(saved);
	const auto parsedParticles = CollectSavedParticles(parsed);
	const auto equalDoubles = [](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-12, 1.0e-9); };
	for (size_t index = 0; index < savedParticles.size(); ++index)
	{
		if (!EqualParticleState(savedParticles[index].particle, parsedParticles[index].particle))
		{
			const auto &lhs = savedParticles[index].particle;
			const auto &rhs = parsedParticles[index].particle;
			std::cerr << "particle_mismatch index=" << index << " type=" << lhs.type << '/' << rhs.type
				<< " x=" << lhs.x << '/' << rhs.x << " y=" << lhs.y << '/' << rhs.y
				<< " vx=" << lhs.vx << '/' << rhs.vx << " vy=" << lhs.vy << '/' << rhs.vy
				<< " temp=" << lhs.temp << '/' << rhs.temp << " life=" << lhs.life << '/' << rhs.life
				<< " ctype=" << lhs.ctype << '/' << rhs.ctype << " tmp=" << lhs.tmp << '/' << rhs.tmp
				<< " tmp2=" << lhs.tmp2 << '/' << rhs.tmp2 << " tmp3=" << lhs.tmp3 << '/' << rhs.tmp3
				<< " tmp4=" << lhs.tmp4 << '/' << rhs.tmp4 << " flags=" << lhs.flags << '/' << rhs.flags
				<< " dcolour=" << lhs.dcolour << '/' << rhs.dcolour << std::endl;
			throw std::runtime_error("serialized particle state changed at index " + std::to_string(index));
		}
		const auto savedIndex = savedParticles[index].sourceIndex;
		const auto parsedIndex = parsedParticles[index].sourceIndex;
		if (saved.hasOmniWaterParcelState &&
			(!equalDoubles(saved.omniWaterParcelMassKg[savedIndex], parsed.omniWaterParcelMassKg[parsedIndex]) ||
			 !equalDoubles(saved.omniWaterParcelSpecificEnthalpyJPerKg[savedIndex],
				parsed.omniWaterParcelSpecificEnthalpyJPerKg[parsedIndex])))
			throw std::runtime_error("serialized water sidecar state changed");
		if (saved.hasOmniCarbonParcelState &&
			!equalDoubles(saved.omniCarbonParcelMassKg[savedIndex], parsed.omniCarbonParcelMassKg[parsedIndex]))
			throw std::runtime_error("serialized carbon sidecar state changed");
		if (saved.hasOmniSolutionState &&
			(!equalDoubles(saved.omniSolutionSolventMassKg[savedIndex], parsed.omniSolutionSolventMassKg[parsedIndex]) ||
			 !equalDoubles(saved.omniSolutionSoluteMassKg[savedIndex], parsed.omniSolutionSoluteMassKg[parsedIndex]) ||
			 !equalDoubles(saved.omniSolutionNeutralSaltMassKg[savedIndex],
				parsed.omniSolutionNeutralSaltMassKg[parsedIndex])))
			throw std::runtime_error("serialized solution sidecar state changed");
		if (saved.hasOmniCorrosionState &&
			(!equalDoubles(saved.omniCorrosionProgress[savedIndex], parsed.omniCorrosionProgress[parsedIndex]) ||
			 !equalDoubles(saved.omniCorrosionPassivation[savedIndex], parsed.omniCorrosionPassivation[parsedIndex])))
			throw std::runtime_error("serialized corrosion sidecar state changed");
	}
	RequireSerializedLegacyStateEquality(saved, parsed);
	if (saved.omniSimulationMode != parsed.omniSimulationMode ||
		saved.hasOmniAtmosphereState != parsed.hasOmniAtmosphereState ||
		saved.omniAtmosphereStateVersion != parsed.omniAtmosphereStateVersion ||
		saved.omniAtmosphereSpecies != parsed.omniAtmosphereSpecies ||
		saved.hasOmniWaterParcelState != parsed.hasOmniWaterParcelState ||
		saved.omniWaterParcelStateVersion != parsed.omniWaterParcelStateVersion ||
		saved.hasOmniCarbonParcelState != parsed.hasOmniCarbonParcelState ||
		saved.omniCarbonParcelStateVersion != parsed.omniCarbonParcelStateVersion ||
		saved.hasOmniSolutionState != parsed.hasOmniSolutionState ||
		saved.omniSolutionStateVersion != parsed.omniSolutionStateVersion ||
		saved.hasOmniCorrosionState != parsed.hasOmniCorrosionState ||
		saved.omniCorrosionStateVersion != parsed.omniCorrosionStateVersion)
		throw std::runtime_error("serialized Omni state metadata changed");
	if (!EqualVector(saved.omniAtmosphereSpeciesMassDensity, parsed.omniAtmosphereSpeciesMassDensity, equalDoubles) ||
		!EqualVector(saved.omniAtmosphereMomentumX, parsed.omniAtmosphereMomentumX, equalDoubles) ||
		!EqualVector(saved.omniAtmosphereMomentumY, parsed.omniAtmosphereMomentumY, equalDoubles) ||
		!EqualVector(saved.omniAtmosphereTotalEnergy, parsed.omniAtmosphereTotalEnergy, equalDoubles) ||
		!EqualVector(saved.omniAtmosphereCondensedWaterDensity, parsed.omniAtmosphereCondensedWaterDensity, equalDoubles) ||
		!EqualVector(saved.omniAtmosphereCellValid, parsed.omniAtmosphereCellValid,
			[](unsigned char lhs, unsigned char rhs) { return lhs == rhs; }))
		throw std::runtime_error("serialized Omni state payload changed");
}

GameSave RoundTripThroughCodec(const GameSave &state)
{
	const auto serialised = state.Serialise();
	if (serialised.first || serialised.second.empty())
		throw std::runtime_error("OPS attack serialization failed");
	return GameSave(serialised.second);
}

void RequireReloadedLegacyStateEquality(const GameSave &parsed, const Simulation &reloaded,
	const Snapshot &expected, const Snapshot &actual)
{
	if (parsed.blockSize != Vec2<int>{ XCELLS, YCELLS })
		throw std::runtime_error("whole-save reload did not preserve full field dimensions");
	const auto equalByte = [](auto lhs, auto rhs) { return lhs == rhs; };
	const auto equalOpsField = [](float lhs, float rhs) {
		return NearlyEqual(lhs, rhs, (1.0 / 128.0) + 1.0e-5, 1.0e-6);
	};
	const auto equalFan = [](float lhs, float rhs) {
		return NearlyEqual(lhs, rhs, (1.0 / 64.0) + 1.0e-6, 1.0e-6);
	};
	const auto equalAmbient = [](float lhs, float rhs) {
		return NearlyEqual(lhs, rhs, 0.50001, 1.0e-6);
	};
	const auto equalGravity = [](float lhs, float rhs) {
		return NearlyEqual(lhs, rhs, 1.0e-6, 1.0e-6);
	};
	if (!EqualVector(expected.BlockMap, actual.BlockMap, equalByte))
		throw std::runtime_error("reload wall map changed");
	if (!EqualVector(expected.FanVelocityX, actual.FanVelocityX, equalFan) ||
		!EqualVector(expected.FanVelocityY, actual.FanVelocityY, equalFan))
		throw std::runtime_error("reload fan velocity changed");
	if (!EqualVector(expected.AirPressure, actual.AirPressure, equalOpsField) ||
		!EqualVector(expected.AirVelocityX, actual.AirVelocityX, equalOpsField) ||
		!EqualVector(expected.AirVelocityY, actual.AirVelocityY, equalOpsField))
		throw std::runtime_error("reload pressure or air velocity changed");
	if (parsed.hasAmbientHeat &&
		!EqualVector(expected.AmbientHeat, actual.AmbientHeat, equalAmbient))
		throw std::runtime_error("reload ambient heat changed");
	if (parsed.hasBlockAirMaps &&
		(!EqualVector(expected.BlockAir, actual.BlockAir, equalByte) ||
		 !EqualVector(expected.BlockAirH, actual.BlockAirH, equalByte)))
		throw std::runtime_error("reload block-air maps changed");
	if (parsed.hasGravityMaps && parsed.gravityEnable &&
		(!EqualVector(expected.GravMass, actual.GravMass, equalGravity) ||
		 !EqualVector(expected.GravMask, actual.GravMask, equalByte) ||
		 !EqualVector(expected.GravForceX, actual.GravForceX, equalGravity) ||
		 !EqualVector(expected.GravForceY, actual.GravForceY, equalGravity)))
		throw std::runtime_error("reload gravity maps changed");
	if (!EqualVector(parsed.signs, actual.signs, EqualSignState))
		throw std::runtime_error("reload signs changed");
	if (parsed.ensureDeterminism &&
		(!parsed.hasRngState || parsed.frameCount != actual.FrameCount ||
		 parsed.rngState != actual.RngState))
		throw std::runtime_error("reload frame or RNG state changed");

	if (parsed.gravityMode != reloaded.gravityMode ||
		!NearlyEqual(parsed.customGravityX, reloaded.customGravityX, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(parsed.customGravityY, reloaded.customGravityY, 1.0e-6, 1.0e-6) ||
		parsed.airMode != reloaded.air->airMode ||
		!NearlyEqual(parsed.ambientAirTemp, reloaded.air->ambientAirTemp, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(parsed.edgePressure, reloaded.air->edgePressure, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(parsed.edgeVelocityX, reloaded.air->edgeVelocityX, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(parsed.edgeVelocityY, reloaded.air->edgeVelocityY, 1.0e-6, 1.0e-6) ||
		!NearlyEqual(parsed.vorticityCoeff, reloaded.air->vorticityCoeff, 1.0e-6, 1.0e-6) ||
		parsed.convectionMode != reloaded.air->convectionMode ||
		parsed.edgeMode != reloaded.edgeMode || parsed.legacyEnable != bool(reloaded.legacy_enable) ||
		parsed.waterEEnabled != bool(reloaded.water_equal_test) ||
		parsed.aheatEnable != bool(reloaded.aheat_enable) ||
		parsed.gravityEnable != bool(reloaded.grav) ||
		parsed.omniSimulationMode != reloaded.omniSimulationMode ||
		parsed.ensureDeterminism != reloaded.ensureDeterminism)
		throw std::runtime_error("reload simulation options changed");
}

void RequireReloadedStateEquality(const GameSave &parsed, const Simulation &reloaded,
	const Snapshot &expectedSnapshot)
{
	const auto actual = reloaded.CreateSnapshot();
	if (!actual)
		throw std::runtime_error("reload state snapshot failed");
	RequireReloadedLegacyStateEquality(parsed, reloaded, expectedSnapshot, *actual);
	auto loaded = CollectLoadedParticles(reloaded, parsed);
	if (loaded.size() != static_cast<size_t>(parsed.particlesCount))
		throw std::runtime_error("reload particle count changed");
	struct ExpectedParticle
	{
		size_t sourceIndex = 0;
		Particle particle{};
	};
	std::vector<ExpectedParticle> expectedParticles;
	expectedParticles.reserve(static_cast<size_t>(parsed.particlesCount));
	for (int index = 0; index < parsed.particlesCount; ++index)
		expectedParticles.push_back({ static_cast<size_t>(index), parsed.particles[index] });
	std::sort(expectedParticles.begin(), expectedParticles.end(), [](const auto &lhs, const auto &rhs) {
		return ParticleSortKey(lhs.particle) < ParticleSortKey(rhs.particle);
	});
	for (size_t index = 0; index < loaded.size(); ++index)
	{
		if (!EqualParticleState(expectedParticles[index].particle, loaded[index].particle))
			throw std::runtime_error("reload particle state changed");
		const auto sourceIndex = expectedParticles[index].sourceIndex;
		const auto loadedIndex = loaded[index].index;
		const auto equalDoubles = [](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-12, 1.0e-9); };
		if (parsed.hasOmniWaterParcelState &&
			(!equalDoubles(parsed.omniWaterParcelMassKg[sourceIndex], reloaded.GetOmniWaterParcelMassKg(loadedIndex)) ||
			 !equalDoubles(parsed.omniWaterParcelSpecificEnthalpyJPerKg[sourceIndex],
				reloaded.GetOmniWaterParcelSpecificEnthalpyJPerKg(loadedIndex))))
			throw std::runtime_error("reload water sidecar state changed");
		if (parsed.hasOmniCarbonParcelState &&
			!equalDoubles(parsed.omniCarbonParcelMassKg[sourceIndex], reloaded.GetOmniCarbonParcelMassKg(loadedIndex)))
			throw std::runtime_error("reload carbon sidecar state changed");
		if (parsed.hasOmniSolutionState &&
			(!equalDoubles(parsed.omniSolutionSolventMassKg[sourceIndex], reloaded.GetOmniSolutionSolventMassKg(loadedIndex)) ||
			 !equalDoubles(parsed.omniSolutionSoluteMassKg[sourceIndex], reloaded.GetOmniSolutionSoluteMassKg(loadedIndex)) ||
			 !equalDoubles(parsed.omniSolutionNeutralSaltMassKg[sourceIndex], reloaded.GetOmniSolutionNeutralSaltMassKg(loadedIndex))))
			throw std::runtime_error("reload solution sidecar state changed");
		if (parsed.hasOmniCorrosionState &&
			(!equalDoubles(parsed.omniCorrosionProgress[sourceIndex], reloaded.GetOmniCorrosionProgress(loadedIndex)) ||
			 !equalDoubles(parsed.omniCorrosionPassivation[sourceIndex], reloaded.GetOmniCorrosionPassivation(loadedIndex))))
			throw std::runtime_error("reload corrosion sidecar state changed");
		if (expectedParticles[index].particle.type == PT_STKM &&
			(parsed.stkm.rocketBoots1 != reloaded.player.rocketBoots ||
			 parsed.stkm.fan1 != reloaded.player.fan))
			throw std::runtime_error("reload primary stickman flags changed");
		if (expectedParticles[index].particle.type == PT_STKM2 &&
			(parsed.stkm.rocketBoots2 != reloaded.player2.rocketBoots ||
			 parsed.stkm.fan2 != reloaded.player2.fan))
			throw std::runtime_error("reload secondary stickman flags changed");
		if (expectedParticles[index].particle.type == PT_FIGH)
		{
			const auto fighter = loaded[index].particle.tmp;
			if (fighter < 0 || fighter >= MAX_FIGHTERS)
				throw std::runtime_error("reload fighter link is invalid");
			const auto oldFighter = static_cast<unsigned int>(expectedParticles[index].particle.tmp);
			const bool expectedRocketBoots = std::find(parsed.stkm.rocketBootsFigh.begin(),
				parsed.stkm.rocketBootsFigh.end(), oldFighter) != parsed.stkm.rocketBootsFigh.end();
			const bool expectedFan = std::find(parsed.stkm.fanFigh.begin(), parsed.stkm.fanFigh.end(),
				oldFighter) != parsed.stkm.fanFigh.end();
			if (expectedRocketBoots != reloaded.fighters[fighter].rocketBoots ||
				expectedFan != reloaded.fighters[fighter].fan)
				throw std::runtime_error("reload fighter flags changed");
		}
	}
	if (parsed.hasOmniAtmosphereState)
	{
		if (expectedSnapshot.OmniSimulationMode != actual->OmniSimulationMode ||
			!EqualVector(expectedSnapshot.OmniAtmosphereSpeciesMassDensity,
				actual->OmniAtmosphereSpeciesMassDensity,
				[](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-8, 1.0e-7); }) ||
			!EqualVector(expectedSnapshot.OmniAtmosphereMomentumX, actual->OmniAtmosphereMomentumX,
				[](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-8, 1.0e-7); }) ||
			!EqualVector(expectedSnapshot.OmniAtmosphereMomentumY, actual->OmniAtmosphereMomentumY,
				[](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-8, 1.0e-7); }) ||
			!EqualVector(expectedSnapshot.OmniAtmosphereTotalEnergy, actual->OmniAtmosphereTotalEnergy,
				[](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-8, 1.0e-7); }) ||
			!EqualVector(expectedSnapshot.OmniAtmosphereCondensedWaterDensity,
				actual->OmniAtmosphereCondensedWaterDensity,
				[](double lhs, double rhs) { return NearlyEqual(lhs, rhs, 1.0e-8, 1.0e-7); }))
			throw std::runtime_error("reload atmosphere state changed");
	}
}

void RunSerializedStateNegativeTests(const GameSave &saved, const GameSave &parsed)
{
	if (parsed.blockMap.Base.empty() || parsed.pressure.Base.empty())
		throw std::runtime_error("negative state tests require full wall and pressure planes");
	RequireRejected("block_map", [&] {
		auto tampered = parsed;
		tampered.blockMap.Base[0] ^= 1;
		const auto reparsed = RoundTripThroughCodec(tampered);
		RequireSerializedStateEquality(saved, reparsed);
	});
	RequireRejected("legacy_pressure", [&] {
		auto tampered = parsed;
		tampered.pressure.Base[0] += 1.0f;
		const auto reparsed = RoundTripThroughCodec(tampered);
		RequireSerializedStateEquality(saved, reparsed);
	});
	RequireRejected("sign", [&] {
		auto tampered = parsed;
		if (tampered.signs.empty())
			tampered.signs.emplace_back(String("negative-probe"), 0, 0, sign::Left);
		else
			tampered.signs.front().x += 1;
		const auto reparsed = RoundTripThroughCodec(tampered);
		RequireSerializedStateEquality(saved, reparsed);
	});
	RequireRejected("omni_validity_mask", [&] {
		auto tampered = parsed;
		if (tampered.omniAtmosphereCellValid.empty())
			tampered.omniAtmosphereCellValid.push_back(1);
		else
			tampered.omniAtmosphereCellValid.front() ^= 1;
		if (tampered.hasOmniAtmosphereState)
		{
			const auto reparsed = RoundTripThroughCodec(tampered);
			RequireSerializedStateEquality(saved, reparsed);
		}
		else
		{
			RequireSerializedStateEquality(saved, tampered);
		}
	});
	RequireRejected("deterministic_frame", [&] {
		auto tampered = parsed;
		if (!tampered.ensureDeterminism || !tampered.hasRngState)
			throw std::runtime_error("deterministic state was not emitted");
		tampered.frameCount += 1;
		const auto reparsed = RoundTripThroughCodec(tampered);
		RequireSerializedStateEquality(saved, reparsed);
	});
	RequireRejected("simulation_option", [&] {
		auto tampered = parsed;
		tampered.edgeMode = (tampered.edgeMode + 1) % NUM_EDGEMODES;
		const auto reparsed = RoundTripThroughCodec(tampered);
		RequireSerializedStateEquality(saved, reparsed);
	});
	RequireRejected("codec_roundtrip", [&] {
		auto tampered = parsed;
		tampered.rngState[0] ^= UINT64_C(1);
		const auto reparsed = RoundTripThroughCodec(tampered);
		RequireSerializedStateEquality(saved, reparsed);
	});
}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: official_save_roundtrip_probe <save>\n";
		return 2;
	}
	try
	{
		std::ifstream input(argv[1], std::ios::binary);
		if (!input)
			throw std::runtime_error("cannot open save fixture");
		std::vector<char> bytes((std::istreambuf_iterator<char>(input)), {});
		// GameSave palette mapping consults SimulationData. Construct the
		// registry first; parsing before it exists dereferences the singleton
		// and crashes with 0xC0000005 instead of producing compatibility evidence.
		GlobalPrefs globalPrefs;
		SimulationData simulationData;
		GameSave source(bytes);
		if (source.missingElements)
			throw std::runtime_error("official save contains unavailable or unmapped elements");
		auto simulation = Simulation::Factory();
		LoadWholeSave(*simulation, source);
		// Bind compatibility to the state immediately produced by the real
		// loader. Simulation::Load intentionally skips particles it cannot
		// instantiate; without this comparison a completely or partially empty
		// world could still survive the later self-consistent save/reload checks.
		auto initialLoadedState = simulation->Save(
			true, RectSized(Vec2{ 0, 0 }, source.blockSize * CELL));
		if (!initialLoadedState)
			throw std::runtime_error("initial loaded-state capture failed");
		// Save(true) emits current-format optional planes even when an old
		// upstream fixture omitted them. Compare only the source-declared planes;
		// paused is GameModel state rather than Simulation state.
		initialLoadedState->hasPressure = source.hasPressure;
		initialLoadedState->hasAmbientHeat = source.hasAmbientHeat && source.aheatEnable;
		initialLoadedState->hasBlockAirMaps = source.hasBlockAirMaps;
		initialLoadedState->hasGravityMaps = source.hasGravityMaps && source.gravityEnable;
		initialLoadedState->paused = source.paused;
		RequireSerializedStateEquality(source, *initialLoadedState);
		const auto initialLoadedParticles = initialLoadedState->particlesCount;
		for (int step = 0; step < 8; ++step)
			Advance(*simulation);
		// Force this compatibility roundtrip to exercise frame/RNG continuation,
		// even when an old upstream fixture did not request deterministic saves.
		simulation->ensureDeterminism = true;
		auto expectedSnapshot = simulation->CreateSnapshot();
		if (!expectedSnapshot)
			throw std::runtime_error("simulation state snapshot failed");
		auto saved = simulation->Save(true, RES.OriginRect());
		if (!saved)
			throw std::runtime_error("simulation save failed");
		auto serialised = saved->Serialise();
		if (serialised.first || serialised.second.empty())
			throw std::runtime_error("roundtrip serialization failed");
		GameSave parsed(serialised.second);
		RequireSerializedStateEquality(*saved, parsed);
		RunSerializedStateNegativeTests(*saved, parsed);
		auto reloaded = Simulation::Factory();
		LoadWholeSave(*reloaded, parsed);
		RequireReloadedStateEquality(parsed, *reloaded, *expectedSnapshot);
		Advance(*reloaded);
		const auto postAdvanceSnapshot = reloaded->CreateSnapshot();
		if (!postAdvanceSnapshot)
			throw std::runtime_error("roundtrip reload validation failed");
		std::cout << "official_save_roundtrip_pass=true\n"
			<< "official_save_load_pass=true\n"
			<< "official_save_missing_elements_zero=true\n"
			<< "official_save_initial_load_state_validate_pass=true\n"
			<< "official_save_simulate_pass=true\n"
			<< "official_save_save_pass=true\n"
			<< "official_save_reload_pass=true\n"
			<< "official_save_state_validate_pass=true\n"
			<< "official_save_negative_block_map_rejected=true\n"
			<< "official_save_negative_legacy_field_rejected=true\n"
			<< "official_save_negative_sign_rejected=true\n"
			<< "official_save_negative_validity_mask_rejected=true\n"
			<< "official_save_negative_deterministic_frame_rejected=true\n"
			<< "official_save_negative_simulation_option_rejected=true\n"
			<< "official_save_negative_codec_roundtrip_rejected=true\n"
			<< "input_particles=" << source.particlesCount << '\n'
			<< "initial_loaded_particles=" << initialLoadedParticles << '\n'
			<< "output_particles=" << parsed.particlesCount << '\n'
			<< "signs=" << parsed.signs.size() << '\n'
			<< "pressure=" << (parsed.hasPressure ? "true" : "false") << '\n'
			<< "ambient_heat=" << (parsed.hasAmbientHeat ? "true" : "false") << '\n'
			<< "omni_mode=" << parsed.omniSimulationMode << '\n'
			<< "omni_atmosphere_state=" << (parsed.hasOmniAtmosphereState ? "true" : "false") << '\n'
			<< "omni_validity_cells=" << parsed.omniAtmosphereCellValid.size() << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << "official-save-roundtrip-probe: FAIL " << error.what() << '\n';
		return 1;
	}
}
