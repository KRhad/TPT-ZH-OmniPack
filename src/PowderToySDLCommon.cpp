#include "PowderToySDL.h"
#include "gui/interface/Engine.h"
#include "common/platform/StressExitTrace.h"

void MainLoop()
{
	OmniStressExitTrace::Log("MainLoop enter running=%d", int(ui::Engine::Ref().Running()));
	unsigned long long iterations = 0;
	while (ui::Engine::Ref().Running())
	{
		++iterations;
		auto delay = EngineProcess();
		if (delay.has_value())
		{
			SDL_Delay(std::max(*delay, UINT64_C(1)));
		}
	}
	OmniStressExitTrace::Log("MainLoop return iterations=%llu running=%d",
		iterations, int(ui::Engine::Ref().Running()));
}

void ApplyFpsLimit()
{
}
