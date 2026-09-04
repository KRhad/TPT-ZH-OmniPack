#include "Platform.h"
#include "StressExitTrace.h"
#include <cstdlib>
#include <list>

namespace Platform
{
std::list<ExitFunc> exitFuncs;

void Atexit(ExitFunc exitFunc)
{
	exitFuncs.push_front(exitFunc);
}

void Exit(int code)
{
	OmniStressExitTrace::Log("Platform::Exit begin code=%d handlers=%zu", code, exitFuncs.size());
	for (auto exitFunc : exitFuncs)
	{
		OmniStressExitTrace::Log("Platform::Exit handler begin");
		exitFunc();
		OmniStressExitTrace::Log("Platform::Exit handler end");
	}
	OmniStressExitTrace::Log("Platform::Exit calling std::exit code=%d", code);
	exit(code);
}
}
