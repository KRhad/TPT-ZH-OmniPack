#pragma once

#include "prefs/GlobalPrefs.h"

struct OmniModuleRuntimeCache
{
	void const *simulation = nullptr;
	int tick = -1;
	bool enabled = true;
};

// Module preferences are global, but particle updates can run hundreds of
// thousands of times per frame.  Cache one preference lookup per simulation
// tick while still applying an options change on the next update tick.
inline bool OmniModuleRuntimeEnabled(
	OmniModuleRuntimeCache &cache,
	void const *simulation,
	int tick,
	char const *preferenceKey)
{
	if (cache.simulation != simulation || cache.tick != tick)
	{
		cache.simulation = simulation;
		cache.tick = tick;
		cache.enabled = GlobalPrefs::Ref().Get(preferenceKey, true);
	}
	return cache.enabled;
}
