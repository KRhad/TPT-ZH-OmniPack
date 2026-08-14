#include "client/GameSave.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace
{
void Advance(Simulation &simulation)
{
	simulation.BeforeSim(true);
	simulation.UpdateParticles(0, simulation.parts.active);
	simulation.AfterSim();
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
		GameSave source(bytes);
		SimulationData simulationData;
		auto simulation = Simulation::Factory();
		simulation->Load(&source, true, { 0, 0 });
		for (int step = 0; step < 8; ++step)
			Advance(*simulation);
		auto saved = simulation->Save(true, RES.OriginRect());
		if (!saved)
			throw std::runtime_error("simulation save failed");
		auto serialised = saved->Serialise();
		if (serialised.first || serialised.second.empty())
			throw std::runtime_error("roundtrip serialization failed");
		GameSave parsed(serialised.second);
		auto reloaded = Simulation::Factory();
		reloaded->Load(&parsed, true, { 0, 0 });
		Advance(*reloaded);
		const auto snapshot = reloaded->CreateSnapshot();
		if (!snapshot || reloaded->parts.active < 0)
			throw std::runtime_error("roundtrip reload validation failed");
		std::cout << "official_save_roundtrip_pass=true\n"
			<< "input_particles=" << source.particlesCount << '\n'
			<< "output_particles=" << parsed.particlesCount << '\n'
			<< "signs=" << parsed.signs.size() << '\n'
			<< "pressure=" << (parsed.hasPressure ? "true" : "false") << '\n'
			<< "ambient_heat=" << (parsed.hasAmbientHeat ? "true" : "false") << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << "official-save-roundtrip-probe: FAIL " << error.what() << '\n';
		return 1;
	}
}
