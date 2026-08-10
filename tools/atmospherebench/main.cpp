#include "AtmosphereBench.h"
#include "Rusanov1D.h"

#include <iostream>
#include <string_view>

int main(int argc, char **argv)
{
	using namespace omni::atmospherebench;
	if (argc == 1 || std::string_view(argv[1]) == "--self-test")
		return RunSelfTest(std::cout) ? 0 : 1;
	if (std::string_view(argv[1]) == "--list-candidates")
	{
		WriteCandidateList(std::cout);
		return 0;
	}
	if (std::string_view(argv[1]) == "--run-uniform")
	{
		WriteUniformScaffold(std::cout);
		return 0;
	}
	if (std::string_view(argv[1]) == "--run-rusanov-uniform")
		return WriteRusanovUniformProbe(std::cout) ? 0 : 1;
	if (std::string_view(argv[1]) == "--run-rusanov-pressure-pulse")
		return WriteRusanovPressurePulseProbe(std::cout) ? 0 : 1;
	if (std::string_view(argv[1]) == "--run-rusanov-density-advection")
		return WriteRusanovDensityAdvectionProbe(std::cout) ? 0 : 1;
	std::cerr << "usage: atmospherebench [--self-test|--list-candidates|--run-uniform|--run-rusanov-uniform|--run-rusanov-pressure-pulse|--run-rusanov-density-advection]\n";
	return 2;
}
