#include "OmniChemistry.h"

#include "ElementCommon.h"

#include <algorithm>

namespace
{
constexpr int ChemistryReactionsPerFrame = 1536;

struct ReactionBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = ChemistryReactionsPerFrame;
};

thread_local ReactionBudget reactionBudget;

struct Slot
{
	int index = -1;
	int x = -1;
	int y = -1;
};

bool ConsumeReactionBudget(Simulation *sim)
{
	if (reactionBudget.simulation != sim || reactionBudget.tick != sim->currentTick)
	{
		reactionBudget.simulation = sim;
		reactionBudget.tick = sim->currentTick;
		reactionBudget.remaining = ChemistryReactionsPerFrame;
	}
	if (reactionBudget.remaining <= 0)
		return false;
	--reactionBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

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

Slot FindLocalExcluding(
	int x,
	int y,
	int type,
	int firstExcluded,
	int secondExcluded,
	int thirdExcluded,
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
			if (index == firstExcluded || index == secondExcluded
				|| index == thirdExcluded || IsTouched(index, parts, sim))
				continue;
			return { index, x + rx, y + ry };
		}
	}
	return {};
}

Slot FindEmpty(int x, int y, int pmap[YRES][XRES])
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			if (!pmap[y + ry][x + rx])
				return { -1, x + rx, y + ry };
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
}

bool CatalyticCracking(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA)
		return false;

	auto temperature = parts[i].temp;
	if (temperature >= 1100.0f)
	{
		auto kerosene = FindLocal(x, y, PT_KERO, i, parts, pmap, sim);
		if (kerosene.index >= 0 && ConsumeReactionBudget(sim))
		{
			Convert(sim, kerosene, PT_ACTY, parts, temperature);
			Touch(kerosene.index, parts, sim);
			return true;
		}
	}
	if (temperature >= 650.0f)
	{
		auto kerosene = FindLocal(x, y, PT_KERO, i, parts, pmap, sim);
		if (kerosene.index >= 0 && ConsumeReactionBudget(sim))
		{
			Convert(sim, kerosene, PT_GASO, parts, temperature);
			Touch(kerosene.index, parts, sim);
			return true;
		}
	}
	if (temperature >= 500.0f)
	{
		auto oil = FindLocal(x, y, PT_OIL, i, parts, pmap, sim);
		if (oil.index >= 0 && ConsumeReactionBudget(sim))
		{
			Convert(sim, oil, PT_KERO, parts, temperature);
			Touch(oil.index, parts, sim);
			return true;
		}
	}
	return false;
}

bool CatalyticPolymerisation(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA || parts[i].temp < 450.0f || parts[i].temp > 800.0f)
		return false;
	auto first = FindLocal(x, y, PT_ACTY, i, parts, pmap, sim);
	if (first.index < 0)
		return false;
	auto second = FindLocal(x, y, PT_ACTY, first.index, parts, pmap, sim);
	if (second.index < 0 || !ConsumeReactionBudget(sim))
		return false;
	auto temperature = parts[i].temp;
	Convert(sim, first, PT_POLY, parts, temperature);
	Convert(sim, second, PT_POLY, parts, temperature);
	Touch(first.index, parts, sim);
	Touch(second.index, parts, sim);
	return true;
}

bool CatalyticPeroxideDecomposition(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA || parts[i].temp < 350.0f)
		return false;
	auto first = FindLocal(x, y, PT_PERO, i, parts, pmap, sim);
	if (first.index < 0)
		return false;
	auto second = FindLocal(x, y, PT_PERO, first.index, parts, pmap, sim);
	auto empty = FindEmpty(x, y, pmap);
	if (second.index < 0 || empty.x < 0 || !ConsumeReactionBudget(sim))
		return false;
	auto temperature = parts[i].temp;
	auto oxygen = sim->create_part(-1, empty.x, empty.y, PT_O2);
	if (oxygen < 0)
		return false;
	Convert(sim, first, PT_WATR, parts, temperature);
	Convert(sim, second, PT_WATR, parts, temperature);
	parts[oxygen].temp = temperature;
	Touch(oxygen, parts, sim);
	Touch(first.index, parts, sim);
	Touch(second.index, parts, sim);
	return true;
}

bool ChlorineReaction(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CHLR || IsTouched(i, parts, sim) || parts[i].temp < 450.0f)
		return false;
	auto hydrogen = FindLocal(x, y, PT_H2, i, parts, pmap, sim);
	if (hydrogen.index < 0 || !ConsumeReactionBudget(sim))
		return false;
	auto self = Slot{ i, x, y };
	auto temperature = parts[i].temp;
	Convert(sim, self, PT_ACID, parts, temperature);
	Convert(sim, hydrogen, PT_ACID, parts, temperature);
	Touch(i, parts, sim);
	Touch(hydrogen.index, parts, sim);
	return true;
}

bool AmmoniaReaction(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_AMON || IsTouched(i, parts, sim))
		return false;
	auto acid = FindLocal(x, y, PT_ACID, i, parts, pmap, sim);
	if (acid.index < 0 || !ConsumeReactionBudget(sim))
		return false;
	auto self = Slot{ i, x, y };
	auto temperature = (parts[i].temp + parts[acid.index].temp) * 0.5f;
	Convert(sim, self, PT_FERT, parts, temperature);
	Convert(sim, acid, PT_FERT, parts, temperature);
	Touch(i, parts, sim);
	Touch(acid.index, parts, sim);
	return true;
}

bool FertilizerUse(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_FERT || IsTouched(i, parts, sim))
		return false;
	auto plant = FindLocal(x, y, PT_PLNT, i, parts, pmap, sim);
	if (plant.index < 0 || FindLocal(x, y, PT_WATR, i, parts, pmap, sim).index < 0
		|| !ConsumeReactionBudget(sim))
		return false;
	parts[plant.index].life = std::max(parts[plant.index].life, 2);
	auto self = Slot{ i, x, y };
	auto temperature = parts[i].temp;
	Convert(sim, self, PT_DUST, parts, temperature);
	Touch(i, parts, sim);
	Touch(plant.index, parts, sim);
	return true;
}
}

int OmniChemistryElementUpdate(UPDATE_FUNC_ARGS)
{
	if (CatalyticCracking(UPDATE_FUNC_SUBCALL_ARGS)
		|| CatalyticPolymerisation(UPDATE_FUNC_SUBCALL_ARGS)
		|| CatalyticPeroxideDecomposition(UPDATE_FUNC_SUBCALL_ARGS)
		|| ChlorineReaction(UPDATE_FUNC_SUBCALL_ARGS)
		|| AmmoniaReaction(UPDATE_FUNC_SUBCALL_ARGS)
		|| FertilizerUse(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return 0;
}

int OmniChemistryYeastUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_YEST || parts[i].temp < 295.0f || parts[i].temp > 310.0f
		|| IsTouched(i, parts, sim))
		return 0;
	auto plant = FindLocal(x, y, PT_PLNT, i, parts, pmap, sim);
	auto water = FindLocal(x, y, PT_WATR, plant.index, parts, pmap, sim);
	if (plant.index < 0 || water.index < 0 || !ConsumeReactionBudget(sim))
		return 0;
	auto temperature = parts[i].temp;
	Convert(sim, plant, PT_ETHL, parts, temperature);
	Convert(sim, water, PT_CO2, parts, temperature);
	Touch(plant.index, parts, sim);
	Touch(water.index, parts, sim);
	Touch(i, parts, sim);
	return 1;
}

int OmniChemistrySparkUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SPRK || parts[i].ctype != PT_CATA || IsTouched(i, parts, sim))
		return 0;
	auto nitrogen = FindLocal(x, y, PT_LNTG, i, parts, pmap, sim);
	auto hydrogenOne = FindLocal(x, y, PT_H2, nitrogen.index, parts, pmap, sim);
	auto hydrogenTwo = FindLocalExcluding(
		x, y, PT_H2, hydrogenOne.index, nitrogen.index, i, parts, pmap, sim);
	auto hydrogenThree = FindLocalExcluding(
		x, y, PT_H2, hydrogenOne.index, hydrogenTwo.index, nitrogen.index,
		parts, pmap, sim);
	if (nitrogen.index >= 0 && hydrogenOne.index >= 0 && hydrogenTwo.index >= 0
		&& hydrogenThree.index >= 0 && sim->pv[y / CELL][x / CELL] >= 2.0f
		&& parts[i].temp >= 500.0f && parts[i].temp <= 1000.0f
		&& ConsumeReactionBudget(sim))
	{
		auto temperature = parts[i].temp;
		Convert(sim, nitrogen, PT_AMON, parts, temperature);
		Convert(sim, hydrogenOne, PT_AMON, parts, temperature);
		Convert(sim, hydrogenTwo, PT_AMON, parts, temperature);
		Convert(sim, hydrogenThree, PT_AMON, parts, temperature);
		Touch(nitrogen.index, parts, sim);
		Touch(hydrogenOne.index, parts, sim);
		Touch(hydrogenTwo.index, parts, sim);
		Touch(hydrogenThree.index, parts, sim);
		Touch(i, parts, sim);
		return 1;
	}
	auto firstWater = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
	if (firstWater.index < 0)
		return 0;
	auto secondWater = FindLocal(x, y, PT_WATR, firstWater.index, parts, pmap, sim);
	if (secondWater.index < 0)
		return 0;
	auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
	if (oxygen.index >= 0 && parts[i].temp >= 320.0f && parts[i].temp <= 600.0f)
	{
		if (!ConsumeReactionBudget(sim))
			return 0;
		auto temperature = parts[i].temp;
		Convert(sim, firstWater, PT_PERO, parts, temperature);
		Convert(sim, secondWater, PT_PERO, parts, temperature);
		sim->kill_part(oxygen.index);
		Touch(firstWater.index, parts, sim);
		Touch(secondWater.index, parts, sim);
		Touch(i, parts, sim);
		return 1;
	}
	auto empty = FindEmpty(x, y, pmap);
	if (empty.x < 0 || !ConsumeReactionBudget(sim))
		return 0;
	auto temperature = parts[i].temp;
	auto newOxygen = sim->create_part(-1, empty.x, empty.y, PT_O2);
	if (newOxygen < 0)
		return 0;
	Convert(sim, firstWater, PT_H2, parts, temperature);
	Convert(sim, secondWater, PT_H2, parts, temperature);
	parts[newOxygen].temp = temperature;
	Touch(firstWater.index, parts, sim);
	Touch(secondWater.index, parts, sim);
	Touch(newOxygen, parts, sim);
	Touch(i, parts, sim);
	return 1;
}
