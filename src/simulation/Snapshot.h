#ifndef SNAPSHOT
#define SNAPSHOT

#include <array>
#include <deque>
#include <memory>
#include <vector>

#include "SimulationData.h"
#include "Particle.h"
#include "common/tpt-minmax.h"
#include "game/Sign.h"
#include "json/json.h"
#include "simulation/ElementDataContainer.h"

class Simulation;
class Snapshot
{
public:
	std::vector<float> AirPressure;
	std::vector<float> AirVelocityX;
	std::vector<float> AirVelocityY;
	std::vector<float> AmbientHeat;

	std::vector<particle> Particles;

	std::unique_ptr<ElementDataContainer> elementData[PT_NUM];

	std::vector<float> GravVelocityX;
	std::vector<float> GravVelocityY;
	std::vector<float> GravValue;
	std::vector<float> GravMap;

	std::vector<unsigned char> BlockMap;
	std::vector<unsigned char> ElecMap;
	std::vector<unsigned char> BlockAir;
	std::vector<unsigned char> BlockAirH;

	std::vector<float> FanVelocityX;
	std::vector<float> FanVelocityY;

	std::vector<Sign> Signs;

	uint64_t FrameCount;
	std::array<uint64_t, 2> RngState;

	Json::Value Authors;

	Snapshot() = default;

	Snapshot(const Snapshot& other) :
		AirPressure(other.AirPressure),
		AirVelocityX(other.AirVelocityX),
		AirVelocityY(other.AirVelocityY),
		AmbientHeat(other.AmbientHeat),
		Particles(other.Particles),
		GravVelocityX(other.GravVelocityX),
		GravVelocityY(other.GravVelocityY),
		GravValue(other.GravValue),
		GravMap(other.GravMap),
		BlockMap(other.BlockMap),
		ElecMap(other.ElecMap),
		BlockAir(other.BlockAir),
		BlockAirH(other.BlockAirH),
		FanVelocityX(other.FanVelocityX),
		FanVelocityY(other.FanVelocityY),
		Signs(other.Signs),
		FrameCount(other.FrameCount),
		RngState(other.RngState),
		Authors(other.Authors)
	{
		for (int i = 0; i < PT_NUM; i++)
		{
			if (other.elementData[i])
			{
				elementData[i] = other.elementData[i]->Clone();
			}
		}
	}

	uint32_t Hash() const;

	static std::unique_ptr<Snapshot> Create(Simulation * sim);
	static void Restore(Simulation * sim, const Snapshot &snap);
};

#endif // SNAPSHOT
