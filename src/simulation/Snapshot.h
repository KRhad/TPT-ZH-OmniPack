#pragma once
#include "Particle.h"
#include "Sign.h"
#include "Stickman.h"
#include "SimulationSettings.h"
#include "common/tpt-rand.h"
#include <vector>
#include <array>
#include <cstdint>
#include <json/json.h>

class Snapshot
{
public:
	std::vector<float> AirPressure;
	std::vector<float> AirVelocityX;
	std::vector<float> AirVelocityY;
	std::vector<float> AmbientHeat;

	// OmniCore authoritative state is kept separate from the Legacy display
	// projection.  Undo/redo must snapshot both or an Enhanced restore would
	// silently keep the newer gas/species state behind older pv/vx/vy/hv data.
	std::vector<double> OmniAtmosphereSpeciesMassDensity;
	std::vector<double> OmniAtmosphereMomentumX;
	std::vector<double> OmniAtmosphereMomentumY;
	std::vector<double> OmniAtmosphereTotalEnergy;
	std::vector<double> OmniAtmosphereCondensedWaterDensity;
	std::vector<double> OmniWaterParcelMassKg;
	std::vector<double> OmniWaterParcelSpecificEnthalpyJPerKg;
	std::vector<double> OmniCarbonParcelMassKg;
	std::vector<double> OmniSolutionSolventMassKg;
	std::vector<double> OmniSolutionSoluteMassKg;
	std::vector<double> OmniSolutionNeutralSaltMassKg;
	int OmniSimulationMode = OMNI_CLASSIC;
	uint8_t OmniAtmospherePersistenceStatus = 0;

	std::vector<Particle> Particles;

	std::vector<float> GravForceX;
	std::vector<float> GravForceY;
	std::vector<float> GravMass;
	std::vector<uint32_t> GravMask;

	std::vector<unsigned char> BlockMap;
	std::vector<unsigned char> ElecMap;
	std::vector<unsigned char> BlockAir;
	std::vector<unsigned char> BlockAirH;

	std::vector<float> FanVelocityX;
	std::vector<float> FanVelocityY;


	std::vector<Particle> PortalParticles;
	std::vector<int> WirelessData;
	std::vector<playerst> stickmen;
	std::vector<sign> signs;

	uint64_t FrameCount;
	RNG::State RngState;

	uint32_t Hash() const;

	Json::Value Authors;

	virtual ~Snapshot() = default;
};
