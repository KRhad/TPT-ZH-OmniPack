#include "OmniBiology.h"

#include "ElementCommon.h"
#include "OmniModuleRuntime.h"
#include "prefs/GlobalPrefs.h"

#include <algorithm>

namespace
{
constexpr int BiologyEventsPerFrame = 1024;
thread_local OmniModuleRuntimeCache biologyRuntimeCache;

struct ReactionBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = BiologyEventsPerFrame;
};

thread_local ReactionBudget reactionBudget;

struct Slot
{
	int index = -1;
	int x = -1;
	int y = -1;
};

bool IsTouched(int index, Parts &parts, Simulation *sim)
{
	return parts[index].tmp3 == sim->currentTick + 1;
}

void Touch(int index, Parts &parts, Simulation *sim)
{
	parts[index].tmp3 = sim->currentTick + 1;
}

Slot FindLocal(
	int x,
	int y,
	int type,
	int excludedIndex,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != type)
				continue;
			auto index = ID(packed);
			if (index == excludedIndex || IsTouched(index, parts, sim))
				continue;
			return { index, x + rx, y + ry };
		}
	}
	return {};
}

void Convert(Simulation *sim, Slot slot, int type, Parts &parts, float temperature)
{
	sim->part_change_type(slot.index, slot.x, slot.y, type);
	parts[slot.index].temp = temperature;
	parts[slot.index].life = 0;
	parts[slot.index].ctype = 0;
	parts[slot.index].tmp = 0;
	parts[slot.index].tmp2 = 0;
}

bool SimplifiedBiology()
{
	return GlobalPrefs::Ref().Get("Omni.Simulation.SimplifiedBiology", false);
}

bool InGrowthWindow(Particle const &part)
{
	return part.temp >= 285.0f && part.temp <= 315.0f;
}

bool AlgaePhotosynthesis(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_ALGA || IsTouched(i, parts, sim) || !InGrowthWindow(parts[i]))
		return false;
	auto nutrient = FindLocal(x, y, PT_NUTR, i, parts, pmap, sim);
	auto water = FindLocal(x, y, PT_WATR, nutrient.index, parts, pmap, sim);
	auto carbonDioxide = FindLocal(x, y, PT_CO2, nutrient.index, parts, pmap, sim);
	if (nutrient.index < 0 || water.index < 0 || carbonDioxide.index < 0
		|| !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = parts[i].temp;
	Convert(sim, nutrient, SimplifiedBiology() ? PT_HUMS : PT_ALGA, parts, temperature);
	Convert(sim, carbonDioxide, PT_O2, parts, temperature);
	Touch(i, parts, sim);
	Touch(nutrient.index, parts, sim);
	Touch(water.index, parts, sim);
	Touch(carbonDioxide.index, parts, sim);
	return true;
}

bool MyceliumDecomposition(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_MYCL || IsTouched(i, parts, sim) || !InGrowthWindow(parts[i]))
		return false;
	auto wood = FindLocal(x, y, PT_WOOD, i, parts, pmap, sim);
	auto water = FindLocal(x, y, PT_WATR, wood.index, parts, pmap, sim);
	if (wood.index < 0 || water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = parts[i].temp;
	Convert(sim, wood, PT_HUMS, parts, temperature);
	Convert(sim, water, PT_NUTR, parts, temperature);
	Touch(i, parts, sim);
	Touch(wood.index, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

bool HumusFertilizerRecovery(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_HUMS || IsTouched(i, parts, sim) || !InGrowthWindow(parts[i]))
		return false;
	auto fertilizer = FindLocal(x, y, PT_FERT, i, parts, pmap, sim);
	auto water = FindLocal(x, y, PT_WATR, fertilizer.index, parts, pmap, sim);
	if (fertilizer.index < 0 || water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = (parts[i].temp + parts[fertilizer.index].temp + parts[water.index].temp) / 3.0f;
	Convert(sim, Slot{ i, x, y }, PT_NUTR, parts, temperature);
	Convert(sim, fertilizer, PT_DUST, parts, temperature);
	Touch(i, parts, sim);
	Touch(fertilizer.index, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

bool SporeGermination(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SPOR || IsTouched(i, parts, sim) || !InGrowthWindow(parts[i]))
		return false;
	auto nutrient = FindLocal(x, y, PT_NUTR, i, parts, pmap, sim);
	auto water = FindLocal(x, y, PT_WATR, nutrient.index, parts, pmap, sim);
	if (nutrient.index < 0 || water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = parts[i].temp;
	Convert(sim, Slot{ i, x, y }, PT_MYCL, parts, temperature);
	Convert(sim, nutrient, SimplifiedBiology() ? PT_HUMS : PT_MYCL, parts, temperature);
	Touch(i, parts, sim);
	Touch(nutrient.index, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

bool PathogenInfection(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PATH || IsTouched(i, parts, sim) || !InGrowthWindow(parts[i]))
		return false;
	auto host = FindLocal(x, y, PT_ALGA, i, parts, pmap, sim);
	if (host.index < 0)
		host = FindLocal(x, y, PT_MYCL, i, parts, pmap, sim);
	if (host.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = parts[i].temp;
	Convert(sim, host, SimplifiedBiology() ? PT_HUMS : PT_PATH, parts, temperature);
	Touch(i, parts, sim);
	Touch(host.index, parts, sim);
	return true;
}

bool Sterilization(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_STER || IsTouched(i, parts, sim))
		return false;
	auto pathogen = FindLocal(x, y, PT_PATH, i, parts, pmap, sim);
	if (pathogen.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = (parts[i].temp + parts[pathogen.index].temp) * 0.5f;
	Convert(sim, Slot{ i, x, y }, PT_HUMS, parts, temperature);
	Convert(sim, pathogen, PT_HUMS, parts, temperature);
	Touch(i, parts, sim);
	Touch(pathogen.index, parts, sim);
	return true;
}

bool BiofilmFiltration(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_BIOF || IsTouched(i, parts, sim))
		return false;
	auto pathogen = FindLocal(x, y, PT_PATH, i, parts, pmap, sim);
	auto water = FindLocal(x, y, PT_WATR, pathogen.index, parts, pmap, sim);
	if (pathogen.index < 0 || water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;

	auto temperature = parts[i].temp;
	Convert(sim, pathogen, PT_HUMS, parts, temperature);
	Touch(i, parts, sim);
	Touch(pathogen.index, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}
}

bool OmniBiologyModuleEnabled(Simulation *sim)
{
	return OmniModuleRuntimeEnabled(
		biologyRuntimeCache, sim, sim->currentTick, "Omni.Modules.Biology");
}

bool OmniConsumeBiologyEvent(Simulation *sim)
{
	if (reactionBudget.simulation != sim || reactionBudget.tick != sim->currentTick)
	{
		reactionBudget.simulation = sim;
		reactionBudget.tick = sim->currentTick;
		reactionBudget.remaining = BiologyEventsPerFrame;
	}
	if (reactionBudget.remaining <= 0)
		return false;
	--reactionBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

int OmniBiologyElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniBiologyModuleEnabled(sim))
		return 0;
	if (AlgaePhotosynthesis(UPDATE_FUNC_SUBCALL_ARGS)
		|| MyceliumDecomposition(UPDATE_FUNC_SUBCALL_ARGS)
		|| HumusFertilizerRecovery(UPDATE_FUNC_SUBCALL_ARGS)
		|| SporeGermination(UPDATE_FUNC_SUBCALL_ARGS)
		|| PathogenInfection(UPDATE_FUNC_SUBCALL_ARGS)
		|| Sterilization(UPDATE_FUNC_SUBCALL_ARGS)
		|| BiofilmFiltration(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return 0;
}
