#include "OmniNuclear.h"

#include "ElementCommon.h"

#include <algorithm>

namespace
{
constexpr int NuclearEventsPerFrame = 512;
constexpr int NuclearSparkEmitted = 0x1;

struct ReactionBudget { Simulation *simulation = nullptr; int tick = -1; int remaining = NuclearEventsPerFrame; };
thread_local ReactionBudget reactionBudget;

struct Slot { int index = -1; int x = -1; int y = -1; };

bool ConsumeEvent(Simulation *sim)
{
	if (reactionBudget.simulation != sim || reactionBudget.tick != sim->currentTick)
	{
		reactionBudget = { sim, sim->currentTick, NuclearEventsPerFrame };
	}
	if (reactionBudget.remaining <= 0)
		return false;
	--reactionBudget.remaining;
	return true;
}

bool IsTouched(int index, Parts &parts, Simulation *sim) { return parts[index].tmp3 == sim->currentTick + 1; }
void Touch(int index, Parts &parts, Simulation *sim) { parts[index].tmp3 = sim->currentTick + 1; }

Slot FindLocal(int x, int y, int type, int excluded, Parts &parts, int pmap[YRES][XRES], Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != type)
				continue;
			auto index = ID(packed);
			if (index != excluded && !IsTouched(index, parts, sim))
				return { index, x + rx, y + ry };
		}
	return {};
}

Slot FindLocalNeutron(int x, int y, Parts &parts, int pmap[YRES][XRES], Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
					continue;
				auto index = ID(packed);
				if (!IsTouched(index, parts, sim))
					return { index, x + rx, y + ry };
			}
		}
	return {};
}

Slot FindEmpty(int x, int y, int pmap[YRES][XRES], int photons[YRES][XRES])
{
	for (int ry = -1; ry <= 1; ++ry)
		for (int rx = -1; rx <= 1; ++rx)
			if ((rx || ry) && InBounds(x + rx, y + ry)
				&& !pmap[y + ry][x + rx] && !photons[y + ry][x + rx])
				return { -1, x + rx, y + ry };
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

bool FuelFission(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_NFUL || IsTouched(i, parts, sim)
		|| parts[i].temp < 285.0f || parts[i].temp > 1500.0f)
		return false;
	auto neutron = FindLocalNeutron(x, y, parts, pmap, sim);
	auto moderator = FindLocal(x, y, PT_MODR, i, parts, pmap, sim);
	auto controlRod = FindLocal(x, y, PT_CROD, i, parts, pmap, sim);
	if (neutron.index < 0 || moderator.index < 0 || controlRod.index >= 0 || !ConsumeEvent(sim))
		return false;
	sim->kill_part(neutron.index);
	Convert(sim, { i, x, y }, PT_NWST, parts, 1700.0f);
	Touch(i, parts, sim);
	Touch(moderator.index, parts, sim);
	return true;
}

bool CoolantBoil(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_NCLT || IsTouched(i, parts, sim))
		return false;
	auto waste = FindLocal(x, y, PT_NWST, i, parts, pmap, sim);
	if (waste.index < 0 || parts[waste.index].temp < 1000.0f || !ConsumeEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_WTRV, parts, parts[waste.index].temp);
	parts[waste.index].temp = 900.0f;
	Touch(i, parts, sim);
	Touch(waste.index, parts, sim);
	return true;
}

bool ShieldAbsorption(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_RSHD || IsTouched(i, parts, sim))
		return false;
	auto neutron = FindLocalNeutron(x, y, parts, pmap, sim);
	if (neutron.index < 0 || !ConsumeEvent(sim))
		return false;
	sim->kill_part(neutron.index);
	parts[i].temp = std::min(parts[i].temp + 60.0f, 1600.0f);
	Touch(i, parts, sim);
	return true;
}
}

int OmniNuclearElementUpdate(UPDATE_FUNC_ARGS)
{
	if (FuelFission(UPDATE_FUNC_SUBCALL_ARGS)
		|| CoolantBoil(UPDATE_FUNC_SUBCALL_ARGS)
		|| ShieldAbsorption(UPDATE_FUNC_SUBCALL_ARGS))
		return 1;
	return 0;
}

int OmniNuclearSparkUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SPRK || parts[i].ctype != PT_NGEN
		|| (parts[i].tmp4 & NuclearSparkEmitted) || IsTouched(i, parts, sim))
		return 0;
	auto fuel = FindLocal(x, y, PT_NFUL, i, parts, pmap, sim);
	if (fuel.index < 0)
		return 0;
	auto empty = FindEmpty(fuel.x, fuel.y, pmap, sim->photons);
	if (empty.x < 0 || !ConsumeEvent(sim))
		return 0;
	auto neutron = sim->create_part(-1, empty.x, empty.y, PT_NEUT);
	if (neutron < 0)
		return 0;
	parts[neutron].temp = parts[i].temp;
	parts[neutron].vx = 0.0f;
	parts[neutron].vy = 0.0f;
	parts[i].tmp4 |= NuclearSparkEmitted;
	Touch(neutron, parts, sim);
	Touch(i, parts, sim);
	return 1;
}
