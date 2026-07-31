#include "bzip2/bz2wrap.h"
#include "client/GameSave.h"
#include "simulation/SimulationData.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

namespace
{
int Fail(std::string_view message)
{
	std::cerr << "legacy-progress-save-probe: FAIL " << message << std::endl;
	return 1;
}

std::vector<char> ReadFile(char const *path)
{
	std::ifstream stream(path, std::ios::binary);
	return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
}

bool OpsPayloadContains(std::vector<char> const &ops, std::string_view marker)
{
	if (ops.size() <= 12 || std::string_view(ops.data(), 4) != "OPS1")
	{
		return false;
	}
	auto payloadSize = static_cast<std::uint32_t>(static_cast<unsigned char>(ops[8])) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[9])) << 8U) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[10])) << 16U) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[11])) << 24U);
	std::vector<char> payload;
	if (BZ2WDecompress(payload, std::span(ops.data() + 12, ops.size() - 12), payloadSize) != BZ2WDecompressOk)
	{
		return false;
	}
	return std::search(payload.begin(), payload.end(), marker.begin(), marker.end()) != payload.end();
}

bool SameStableParticleReferences(GameSave const &before, GameSave const &after)
{
	if (before.particlesCount != after.particlesCount)
	{
		return false;
	}
	for (int index = 0; index < before.particlesCount; ++index)
	{
		auto const &left = before.particles[index];
		auto const &right = after.particles[index];
		if (left.type != right.type || left.ctype != right.ctype || left.tmp != right.tmp || left.tmp2 != right.tmp2)
		{
			return false;
		}
	}
	return true;
}
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		return Fail("no legacy OPS samples were supplied");
	}

	SimulationData simulationData;
	int samples = 0;
	int particles = 0;
	for (int argument = 1; argument < argc; ++argument)
	{
		auto source = ReadFile(argv[argument]);
		if (source.empty())
		{
			return Fail("cannot read a legacy OPS sample");
		}
		if (!OpsPayloadContains(source, std::string_view("omniAlchemy\0", 12)))
		{
			return Fail("sample does not contain the retired omniAlchemy field");
		}

		GameSave loaded(source);
		auto serialised = loaded.Serialise().second;
		if (serialised.empty())
		{
			return Fail("reserialising a legacy save returned no OPS data");
		}
		if (OpsPayloadContains(serialised, std::string_view("omniAlchemy\0", 12)))
		{
			return Fail("new OPS output still writes the retired omniAlchemy field");
		}

		GameSave reloaded(serialised);
		if (loaded.blockSize != reloaded.blockSize || !SameStableParticleReferences(loaded, reloaded))
		{
			return Fail("legacy save particles or indirect type fields changed after resave");
		}
		++samples;
		particles += loaded.particlesCount;
	}

	std::cout << "legacy-progress-save-probe: PASS samples=" << samples
		<< " particles=" << particles
		<< " legacy_field_read_ignored=true legacy_field_written=false particle_types_preserved=true"
		<< std::endl;
	return 0;
}
