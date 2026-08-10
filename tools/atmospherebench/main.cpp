#include "AtmosphereBench.h"

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
	std::cerr << "usage: atmospherebench [--self-test|--list-candidates|--run-uniform]\n";
	return 2;
}
