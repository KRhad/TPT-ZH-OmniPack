#include "OmniMetallurgy.h"

#include "ElementCommon.h"
#include "OmniModuleRuntime.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
thread_local OmniModuleRuntimeCache metallurgyRuntimeCache;

bool MetallurgyModuleEnabled(Simulation *sim)
{
	return OmniModuleRuntimeEnabled(
		metallurgyRuntimeCache, sim, sim->currentTick, "Omni.Modules.Metallurgy");
}

enum class PhaseMatch : unsigned char
{
	Direct,
	Molten,
	DirectOrMolten,
};

struct Ingredient
{
	int material;
	int count;
};

struct AlloyRecipe
{
	std::array<Ingredient, 3> ingredients;
	int ingredientCount;
	int product;
	float minimumTemperature;
};

// More specific recipes precede overlapping simpler ratios.  For example a
// 3:2 copper/nickel charge must become constantan before the 3:1 cupronickel
// recipe is considered, and nickel superalloy must precede nichrome.
constexpr std::array<AlloyRecipe, 14> AlloyRecipes{ {
	{ { Ingredient{ PT_STEL, 4 }, Ingredient{ PT_CHRM, 1 }, Ingredient{ PT_NICL, 1 } }, 3, PT_SSIL, 2200.0f },
	{ { Ingredient{ PT_STEL, 4 }, Ingredient{ PT_COBT, 1 }, Ingredient{ PT_MOLY, 1 } }, 3, PT_TSTL, 2950.0f },
	{ { Ingredient{ PT_NICL, 4 }, Ingredient{ PT_CHRM, 1 }, Ingredient{ PT_COBT, 1 } }, 3, PT_NSAL, 2250.0f },
	{ { Ingredient{ PT_TTAN, 4 }, Ingredient{ PT_ALUM, 1 }, Ingredient{ PT_V,    1 } }, 3, PT_TIAL, 2250.0f },
	{ { Ingredient{ PT_TUNG, 4 }, Ingredient{ PT_NICL, 1 }, Ingredient{ PT_IRON, 1 } }, 3, PT_WALY, 3800.0f },
	{ { Ingredient{ PT_ZR,   4 }, Ingredient{ PT_TIN,  1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_ZRAL, 2250.0f },
	{ { Ingredient{ PT_COPR, 3 }, Ingredient{ PT_NICL, 2 }, Ingredient{ PT_NONE, 0 } }, 2, PT_CNST, 1800.0f },
	{ { Ingredient{ PT_COPR, 3 }, Ingredient{ PT_NICL, 1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_CUNI, 1800.0f },
	{ { Ingredient{ PT_NICL, 1 }, Ingredient{ PT_TTAN, 1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_NITI, 2050.0f },
	{ { Ingredient{ PT_COPR, 3 }, Ingredient{ PT_TIN,  1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_BRNZ, 1375.0f },
	{ { Ingredient{ PT_COPR, 3 }, Ingredient{ PT_ZINC, 1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_BRAS, 1375.0f },
	{ { Ingredient{ PT_NICL, 4 }, Ingredient{ PT_CHRM, 1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_NCRM, 2200.0f },
	{ { Ingredient{ PT_ALUM, 4 }, Ingredient{ PT_MAGN, 1 }, Ingredient{ PT_NONE, 0 } }, 2, PT_ALMG,  950.0f },
	{ { Ingredient{ PT_TIN,  3 }, Ingredient{ PT_LEAD, 2 }, Ingredient{ PT_NONE, 0 } }, 2, PT_SOLD,  700.0f },
} };

constexpr int MaxLocalParticles = 9;
constexpr int ReactionsPerFrame = 2048;

struct LocalParticle
{
	int index = -1;
	int x = 0;
	int y = 0;
	int material = PT_NONE;
	bool molten = false;
};

struct LocalParticles
{
	std::array<LocalParticle, MaxLocalParticles> items{};
	int count = 0;
};

struct Selection
{
	std::array<int, MaxLocalParticles> localIndices{};
	int count = 0;
};

struct ReactionBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = ReactionsPerFrame;
};

thread_local ReactionBudget reactionBudget;

int FindNeighbour(
	int x,
	int y,
	int type,
	int ctype,
	Parts &parts,
	int pmap[YRES][XRES])
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != type)
			{
				continue;
			}
			auto neighbour = ID(packed);
			if (ctype == PT_NONE || parts[neighbour].ctype == ctype)
			{
				return neighbour;
			}
		}
	}
	return -1;
}

int FindEnergyNeighbour(int x, int y, int type, Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if (!InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = sim->photons[y + ry][x + rx];
			if (packed && TYP(packed) == type)
			{
				return ID(packed);
			}
		}
	}
	return -1;
}

int FindNeighbourAt(
	int x,
	int y,
	int type,
	int ctype,
	Parts &parts,
	int pmap[YRES][XRES],
	int &outX,
	int &outY)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != type)
			{
				continue;
			}
			auto neighbour = ID(packed);
			if (ctype == PT_NONE || parts[neighbour].ctype == ctype)
			{
				outX = x + rx;
				outY = y + ry;
				return neighbour;
			}
		}
	}
	return -1;
}

bool HasNeighbour(
	int x,
	int y,
	int type,
	Parts &parts,
	int pmap[YRES][XRES])
{
	return FindNeighbour(x, y, type, PT_NONE, parts, pmap) >= 0;
}

bool HasOxygen(int x, int y, Parts &parts, int pmap[YRES][XRES])
{
	return HasNeighbour(x, y, PT_O2, parts, pmap)
		|| HasNeighbour(x, y, PT_LO2, parts, pmap);
}

bool IsMoltenMetallurgyComponent(int ctype)
{
	switch (ctype)
	{
	case PT_IRON:
	case PT_ALUM:
	case PT_COPR:
	case PT_LEAD:
	case PT_TIN:
	case PT_NICL:
	case PT_MAGN:
	case PT_CHRM:
	case PT_COBT:
	case PT_MOLY:
	case PT_ZINC:
	case PT_STEL:
	case PT_SOLD:
	case PT_TTAN:
	case PT_V:
	case PT_TUNG:
	case PT_ZR:
	case PT_CSTI:
	case PT_CUNI:
	case PT_TIAL:
	case PT_NSAL:
	case PT_WALY:
	case PT_ZRAL:
	case PT_CNST:
	case PT_NITI:
		return true;
	default:
		return false;
	}
}

float BreakPressureFor(int type)
{
	switch (type)
	{
	case PT_LEAD: return 12.0f;
	case PT_TIN:  return 18.0f;
	case PT_SOLD: return 16.0f;
	case PT_MAGN:
	case PT_ZINC: return 20.0f;
	case PT_ALUM: return 25.0f;
	case PT_ALMG: return 45.0f;
	case PT_CSTI: return 65.0f;
	case PT_COPR: return 48.0f;
	case PT_BRAS: return 60.0f;
	case PT_NCRM: return 70.0f;
	case PT_CNST: return 82.0f;
	case PT_BRNZ: return 75.0f;
	case PT_NICL: return 82.0f;
	case PT_CUNI: return 100.0f;
	case PT_COBT: return 90.0f;
	case PT_CHRM:
	case PT_STEL: return 105.0f;
	case PT_MOLY:
	case PT_SSIL: return 125.0f;
	case PT_ZRAL: return 145.0f;
	case PT_TSTL: return 155.0f;
	case PT_NITI: return 170.0f;
	case PT_TIAL: return 180.0f;
	case PT_NSAL: return 195.0f;
	case PT_WALY: return 220.0f;
	default:      return 0.0f;
	}
}

bool BreakIntoScrap(
	int i,
	int x,
	int y,
	Parts &parts,
	Simulation *sim)
{
	auto threshold = BreakPressureFor(parts[i].type);
	if (threshold <= 0.0f
		|| std::abs(sim->pv[y / CELL][x / CELL]) < threshold)
	{
		return false;
	}
	auto sourceType = parts[i].type;
	auto temperature = parts[i].temp;
	if (sim->part_change_type(i, x, y, PT_BRMT))
	{
		return true;
	}
	MarkOmniRecoverableScrap(parts[i], sourceType);
	parts[i].temp = temperature;
	return true;
}

void ResetMoltenMetadata(Particle &part)
{
	part.tmp = 0;
	part.tmp2 = 0;
	part.life = std::max(part.life, 20);
}

bool IsRecoverableSourceType(int type)
{
	return BreakPressureFor(type) > 0.0f
		|| type == PT_SC || type == PT_V || type == PT_MN
		|| type == PT_Y || type == PT_ZR || type == PT_NB || type == PT_TC
		|| type == PT_RU || type == PT_RH || type == PT_PD || type == PT_AG
		|| type == PT_CD || type == PT_HF || type == PT_RE || type == PT_IR
		|| (type >= PT_LA && type <= PT_LU)
		|| (type >= PT_AC && type <= PT_LR)
		|| (type >= PT_RF && type <= PT_CN);
}

bool ConsumeReactionBudget(Simulation *sim)
{
	if (reactionBudget.simulation != sim || reactionBudget.tick != sim->currentTick)
	{
		reactionBudget.simulation = sim;
		reactionBudget.tick = sim->currentTick;
		reactionBudget.remaining = ReactionsPerFrame;
	}
	if (reactionBudget.remaining <= 0)
	{
		return false;
	}
	--reactionBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

bool TryRadiationShieldAssembly(
	int i,
	int x,
	int y,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	auto touchedMarker = sim->currentTick + 1;
	if (parts[i].type != PT_SSIL
		|| parts[i].temp < 700.0f || parts[i].temp > 1200.0f
		|| parts[i].tmp3 == touchedMarker)
		return false;

	int leadX = 0;
	int leadY = 0;
	auto lead = FindNeighbourAt(
		x, y, PT_LAVA, PT_LEAD, parts, pmap, leadX, leadY);
	auto heater = FindNeighbour(x, y, PT_SPRK, PT_NCRM, parts, pmap);
	if (lead < 0 || heater < 0
		|| parts[lead].temp < 650.0f || parts[lead].temp > 1200.0f
		|| parts[lead].tmp3 == touchedMarker
		|| parts[heater].tmp3 == touchedMarker
		|| !ConsumeReactionBudget(sim))
		return false;

	auto temperature = (parts[i].temp + parts[lead].temp) * 0.5f;
	auto makeShield = [&](int index, int px, int py)
	{
		sim->part_change_type(index, px, py, PT_RSHD);
		parts[index].temp = temperature;
		parts[index].life = 0;
		parts[index].ctype = 0;
		parts[index].tmp = 0;
		parts[index].tmp2 = 0;
		parts[index].tmp4 = 0;
		parts[index].tmp3 = touchedMarker;
	};
	makeShield(i, x, y);
	makeShield(lead, leadX, leadY);
	parts[heater].tmp3 = touchedMarker;
	return true;
}

LocalParticles CollectLocalParticles(
	int i,
	int x,
	int y,
	Parts &parts,
	int pmap[YRES][XRES])
{
	LocalParticles local;
	auto append = [&](int index, int px, int py)
	{
		if (index < 0 || local.count >= MaxLocalParticles)
		{
			return;
		}
		auto &entry = local.items[local.count++];
		entry.index = index;
		entry.x = px;
		entry.y = py;
		entry.molten = parts[index].type == PT_LAVA;
		entry.material = entry.molten ? parts[index].ctype : parts[index].type;
	};

	// The current particle is deliberately first, so a matching recipe always
	// includes the update that triggered it when excess ingredients are nearby.
	append(i, x, y);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (packed)
			{
				append(ID(packed), x + rx, y + ry);
			}
		}
	}
	return local;
}

bool PhaseMatches(LocalParticle const &particle, PhaseMatch phase)
{
	switch (phase)
	{
	case PhaseMatch::Direct:
		return !particle.molten;
	case PhaseMatch::Molten:
		return particle.molten;
	case PhaseMatch::DirectOrMolten:
		return true;
	}
	return false;
}

bool AddIngredient(
	LocalParticles const &local,
	Parts &parts,
	int material,
	int count,
	PhaseMatch phase,
	float minimumTemperature,
	int touchedMarker,
	std::array<bool, MaxLocalParticles> &used,
	Selection &selection)
{
	auto remaining = count;
	for (int localIndex = 0; localIndex < local.count && remaining > 0; ++localIndex)
	{
		auto const &candidate = local.items[localIndex];
		if (used[localIndex]
			|| candidate.material != material
			|| !PhaseMatches(candidate, phase)
			|| parts[candidate.index].temp < minimumTemperature
			|| parts[candidate.index].tmp3 == touchedMarker)
		{
			continue;
		}
		used[localIndex] = true;
		selection.localIndices[selection.count++] = localIndex;
		--remaining;
	}
	return remaining == 0;
}

bool SelectAlloyRecipe(
	AlloyRecipe const &recipe,
	LocalParticles const &local,
	Parts &parts,
	int touchedMarker,
	Selection &selection)
{
	std::array<bool, MaxLocalParticles> used{};
	for (int ingredientIndex = 0;
		ingredientIndex < recipe.ingredientCount;
		++ingredientIndex)
	{
		auto const &ingredient = recipe.ingredients[ingredientIndex];
		if (!AddIngredient(
			local,
			parts,
			ingredient.material,
			ingredient.count,
			PhaseMatch::Molten,
			recipe.minimumTemperature,
			touchedMarker,
			used,
			selection))
		{
			return false;
		}
	}
	return used[0];
}

bool TryAlloyRecipes(
	int i,
	LocalParticles const &local,
	Parts &parts,
	Simulation *sim)
{
	auto touchedMarker = sim->currentTick + 1;
	for (auto const &recipe : AlloyRecipes)
	{
		Selection selection;
		if (!SelectAlloyRecipe(
			recipe, local, parts, touchedMarker, selection))
		{
			continue;
		}
		if (!ConsumeReactionBudget(sim))
		{
			return false;
		}
		float temperature = 0.0f;
		for (int selected = 0; selected < selection.count; ++selected)
		{
			auto const &entry =
				local.items[selection.localIndices[selected]];
			temperature += parts[entry.index].temp;
		}
		temperature /= float(selection.count);
		for (int selected = 0; selected < selection.count; ++selected)
		{
			auto const &entry =
				local.items[selection.localIndices[selected]];
			parts[entry.index].ctype = recipe.product;
			parts[entry.index].temp = temperature;
			ResetMoltenMetadata(parts[entry.index]);
			parts[entry.index].tmp3 = touchedMarker;
			if (recipe.product == PT_TIAL || recipe.product == PT_NITI)
			{
				parts[entry.index].dcolour = 0;
			}
		}
		return true;
	}
	return false;
}

bool TrySteelRecipe(
	int i,
	LocalParticles const &local,
	Parts &parts,
	Simulation *sim)
{
	if (parts[i].ctype != PT_IRON)
	{
		return false;
	}
	auto touchedMarker = sim->currentTick + 1;
	std::array<bool, MaxLocalParticles> used{};
	Selection iron;
	Selection coke;
	Selection flux;
	if (!AddIngredient(
			local, parts, PT_IRON, 4, PhaseMatch::Molten, 1750.0f,
			touchedMarker, used, iron)
		|| !AddIngredient(
			local, parts, PT_COKE, 1, PhaseMatch::Direct, 950.0f,
			touchedMarker, used, coke)
		|| !AddIngredient(
			local, parts, PT_FLUX, 1, PhaseMatch::DirectOrMolten, 1050.0f,
			touchedMarker, used, flux)
		|| !used[0]
		|| !ConsumeReactionBudget(sim))
	{
		return false;
	}

	float steelTemperature = 0.0f;
	for (int selected = 0; selected < iron.count; ++selected)
	{
		auto const &entry = local.items[iron.localIndices[selected]];
		steelTemperature += parts[entry.index].temp;
	}
	steelTemperature /= float(iron.count);
	for (int selected = 0; selected < iron.count; ++selected)
	{
		auto const &entry = local.items[iron.localIndices[selected]];
		parts[entry.index].ctype = PT_STEL;
		parts[entry.index].temp = std::max(steelTemperature, 1800.0f);
		ResetMoltenMetadata(parts[entry.index]);
		parts[entry.index].tmp3 = touchedMarker;
	}

	auto const &cokeEntry = local.items[coke.localIndices[0]];
	sim->part_change_type(
		cokeEntry.index, cokeEntry.x, cokeEntry.y, PT_CO2);
	parts[cokeEntry.index].temp = std::min(steelTemperature, 1400.0f);
	parts[cokeEntry.index].life = 0;
	parts[cokeEntry.index].ctype = PT_NONE;

	auto const &fluxEntry = local.items[flux.localIndices[0]];
	if (parts[fluxEntry.index].type == PT_LAVA)
	{
		parts[fluxEntry.index].ctype = PT_SLAG;
		ResetMoltenMetadata(parts[fluxEntry.index]);
	}
	else
	{
		sim->part_change_type(
			fluxEntry.index, fluxEntry.x, fluxEntry.y, PT_SLAG);
	}
	parts[fluxEntry.index].temp = std::min(steelTemperature, 1450.0f);
	parts[fluxEntry.index].tmp3 = touchedMarker;
	return true;
}

bool TryCastIronRecipe(
	int i,
	LocalParticles const &local,
	Parts &parts,
	Simulation *sim)
{
	if (parts[i].ctype != PT_IRON)
	{
		return false;
	}
	auto touchedMarker = sim->currentTick + 1;
	std::array<bool, MaxLocalParticles> used{};
	Selection iron;
	Selection coke;
	if (!AddIngredient(
			local, parts, PT_IRON, 4, PhaseMatch::Molten, 1750.0f,
			touchedMarker, used, iron)
		|| !AddIngredient(
			local, parts, PT_COKE, 1, PhaseMatch::Direct, 950.0f,
			touchedMarker, used, coke)
		|| !used[0]
		|| !ConsumeReactionBudget(sim))
	{
		return false;
	}

	float castingTemperature = 0.0f;
	for (int selected = 0; selected < iron.count; ++selected)
	{
		auto const &entry = local.items[iron.localIndices[selected]];
		castingTemperature += parts[entry.index].temp;
	}
	castingTemperature /= float(iron.count);
	for (int selected = 0; selected < iron.count; ++selected)
	{
		auto const &entry = local.items[iron.localIndices[selected]];
		parts[entry.index].ctype = PT_CSTI;
		parts[entry.index].temp = castingTemperature;
		parts[entry.index].dcolour = 0;
		ResetMoltenMetadata(parts[entry.index]);
		parts[entry.index].tmp3 = touchedMarker;
	}

	auto const &cokeEntry = local.items[coke.localIndices[0]];
	sim->part_change_type(cokeEntry.index, cokeEntry.x, cokeEntry.y, PT_SMKE);
	parts[cokeEntry.index].temp = std::min(castingTemperature, 500.0f);
	parts[cokeEntry.index].life = 45;
	parts[cokeEntry.index].ctype = PT_NONE;
	parts[cokeEntry.index].tmp = 0;
	parts[cokeEntry.index].tmp2 = 0;
	parts[cokeEntry.index].tmp3 = touchedMarker;
	parts[cokeEntry.index].tmp4 = 0;
	return true;
}
}

bool IsOmniRecoverableScrap(Particle const &particle)
{
	return particle.type == PT_BRMT && particle.tmp4 == OmniRecoverableScrapMarker;
}

bool IsOmniRecoverableMetalType(int type)
{
	return IsRecoverableSourceType(type);
}

void MarkOmniRecoverableScrap(Particle &particle, int sourceType)
{
	particle.life = 0;
	particle.ctype = IsOmniRecoverableMetalType(sourceType) ? sourceType : PT_IRON;
	particle.tmp = 0;
	particle.tmp2 = 0;
	particle.tmp3 = 0;
	particle.tmp4 = OmniRecoverableScrapMarker;
}

int OmniMetallurgyMetalUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim))
		return 0;
	if (TryRadiationShieldAssembly(i, x, y, parts, pmap, sim))
	{
		return 1;
	}
	if (BreakIntoScrap(i, x, y, parts, sim))
	{
		return 1;
	}

	switch (parts[i].type)
	{
	case PT_CSTI:
	{
		int waterX = 0;
		int waterY = 0;
		auto water = FindNeighbourAt(
			x, y, PT_WATR, PT_NONE, parts, pmap, waterX, waterY);
		if (water < 0)
		{
			water = FindNeighbourAt(
				x, y, PT_SLTW, PT_NONE, parts, pmap, waterX, waterY);
		}
		if (parts[i].temp > 850.0f && water >= 0
			&& sim->rng.chance(1, 8) && ConsumeReactionBudget(sim))
		{
			auto temperature = parts[i].temp;
			sim->part_change_type(i, x, y, PT_BRMT);
			MarkOmniRecoverableScrap(parts[i], PT_CSTI);
			parts[i].temp = std::max(temperature - 120.0f, 730.0f);
			sim->part_change_type(water, waterX, waterY, PT_WTRV);
			parts[water].temp = std::max(parts[water].temp, 430.0f);
			return 1;
		}
		break;
	}
	case PT_COPR:
	{
		auto oxygen = FindNeighbour(x, y, PT_O2, PT_NONE, parts, pmap);
		auto saltWater = FindNeighbour(x, y, PT_SLTW, PT_NONE, parts, pmap);
		auto water = FindNeighbour(x, y, PT_WATR, PT_NONE, parts, pmap);
		auto corrosion = oxygen >= 0
			? sim->rng.chance(1, parts[i].temp > 550.0f ? 50 : 350)
			: (saltWater >= 0 && sim->rng.chance(1, 220))
				|| (water >= 0 && sim->rng.chance(1, 1200));
		if (corrosion)
		{
			parts[i].tmp = std::min(parts[i].tmp + 1, 100);
			if (oxygen >= 0 && sim->rng.chance(1, 4))
			{
				sim->kill_part(oxygen);
			}
			if (parts[i].tmp >= 100)
			{
				sim->part_change_type(i, x, y, PT_BRMT);
				MarkOmniRecoverableScrap(parts[i], PT_COPR);
				return 1;
			}
		}
		break;
	}
	case PT_MAGN:
		if (parts[i].temp > 800.0f && sim->rng.chance(1, 12))
		{
			int oxygenX = 0;
			int oxygenY = 0;
			auto oxygen = FindNeighbourAt(
				x, y, PT_O2, PT_NONE, parts, pmap, oxygenX, oxygenY);
			if (oxygen >= 0)
			{
				sim->kill_part(oxygen);
				auto flame = sim->create_part(-1, oxygenX, oxygenY, PT_FIRE);
				if (flame >= 0)
				{
					parts[flame].temp = 2500.0f;
					parts[flame].life = 80;
					parts[flame].dcolour = 0xFFFFFFFF;
				}
				sim->part_change_type(i, x, y, PT_BRMT);
				MarkOmniRecoverableScrap(parts[i], PT_MAGN);
				parts[i].temp = 2200.0f;
				return 1;
			}
		}
		break;
	case PT_ZINC:
	{
		auto iron = FindNeighbour(x, y, PT_IRON, PT_NONE, parts, pmap);
		auto saltWater = FindNeighbour(x, y, PT_SLTW, PT_NONE, parts, pmap);
		auto water = FindNeighbour(x, y, PT_WATR, PT_NONE, parts, pmap);
		if (iron >= 0)
		{
			// IRON skips its corrosion update while life is positive.
			parts[iron].life = std::max(parts[iron].life, 3);
		}
		if ((saltWater >= 0 && sim->rng.chance(1, 90))
			|| (water >= 0 && sim->rng.chance(1, 500)))
		{
			parts[i].tmp = std::min(parts[i].tmp + 1, 100);
			if (parts[i].tmp >= 100)
			{
				sim->part_change_type(i, x, y, PT_BRMT);
				MarkOmniRecoverableScrap(parts[i], PT_ZINC);
				return 1;
			}
		}
		break;
	}
	case PT_TIAL:
	{
		if (parts[i].tmp == 0 && parts[i].temp > 900.0f)
		{
			auto oxygen = FindNeighbour(x, y, PT_O2, PT_NONE, parts, pmap);
			if (oxygen >= 0 && sim->rng.chance(1, 32)
				&& ConsumeReactionBudget(sim))
			{
				sim->kill_part(oxygen);
				parts[i].tmp = 1;
				parts[i].dcolour = 0xFFB8C4D0;
				return 1;
			}
		}
		break;
	}
	case PT_WALY:
	{
		auto neutron = FindEnergyNeighbour(x, y, PT_NEUT, sim);
		if (neutron >= 0 && sim->rng.chance(1, 4)
			&& ConsumeReactionBudget(sim))
		{
			sim->kill_part(neutron);
			parts[i].temp = std::min(parts[i].temp + 40.0f, 3275.0f);
			return 1;
		}
		break;
	}
	case PT_ZRAL:
	{
		if (parts[i].temp <= 1250.0f)
		{
			break;
		}
		int waterX = 0;
		int waterY = 0;
		auto water = FindNeighbourAt(
			x, y, PT_WTRV, PT_NONE, parts, pmap, waterX, waterY);
		if (water < 0)
		{
			water = FindNeighbourAt(
				x, y, PT_WATR, PT_NONE, parts, pmap, waterX, waterY);
		}
		if (water >= 0 && sim->rng.chance(1, 24)
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = parts[i].temp;
			sim->part_change_type(water, waterX, waterY, PT_H2);
			parts[water].temp = std::max(parts[water].temp, 700.0f);
			sim->part_change_type(i, x, y, PT_BRMT);
			MarkOmniRecoverableScrap(parts[i], PT_ZRAL);
			parts[i].temp = std::min(temperature + 120.0f, 1875.0f);
			return 1;
		}
		break;
	}
	case PT_NITI:
	{
		if (parts[i].tmp == 1 && parts[i].temp >= 500.0f)
		{
			if (ConsumeReactionBudget(sim))
			{
				parts[i].tmp = 0;
				parts[i].dcolour = 0;
			}
			break;
		}
		auto pressure = std::abs(sim->pv[y / CELL][x / CELL]);
		if (parts[i].tmp == 0 && pressure >= 55.0f && pressure < 170.0f
			&& ConsumeReactionBudget(sim))
		{
			parts[i].tmp = 1;
			parts[i].dcolour = 0xFF746F79;
			parts[i].temp = std::min(parts[i].temp + 8.0f, 490.0f);
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

int OmniMetallurgyScrapUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim) || !IsOmniRecoverableScrap(parts[i]))
		return 0;
	auto sourceType = parts[i].ctype;
	if (!IsRecoverableSourceType(sourceType))
	{
		parts[i].ctype = PT_IRON;
		sourceType = PT_IRON;
	}
	auto const &source = SimulationData::CRef().elements[sourceType];
	if (source.HighTemperatureTransition != PT_LAVA
		|| parts[i].temp < source.HighTemperature)
	{
		return 0;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_LAVA);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 30;
	parts[i].tmp = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp3 = 0;
	parts[i].tmp4 = 0;
	return 1;
}

int OmniMetallurgyLegacyScrapAliasUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim) || parts[i].type != PT_MSCR)
		return 0;
	auto sourceType = parts[i].ctype;
	auto temperature = parts[i].temp;
	auto dcolour = parts[i].dcolour;
	if (sim->part_change_type(i, x, y, PT_BRMT))
		return 1;
	MarkOmniRecoverableScrap(parts[i], sourceType);
	parts[i].temp = temperature;
	parts[i].dcolour = dcolour;
	return 1;
}

int OmniMetallurgyLavaUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim))
		return 0;
	if (parts[i].type != PT_LAVA
		|| !IsMoltenMetallurgyComponent(parts[i].ctype)
		|| parts[i].tmp3 == sim->currentTick + 1)
	{
		return 0;
	}

	auto local = CollectLocalParticles(i, x, y, parts, pmap);
	if (TrySteelRecipe(i, local, parts, sim))
	{
		return 1;
	}
	if (TryCastIronRecipe(i, local, parts, sim))
	{
		return 1;
	}
	return TryAlloyRecipes(i, local, parts, sim) ? 1 : 0;
}

int OmniMetallurgySparkUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim)
		|| parts[i].type != PT_SPRK
		|| parts[i].ctype != PT_SOLD)
	{
		return 0;
	}
	auto const &solder = SimulationData::CRef().elements[PT_SOLD];
	parts[i].temp = std::min(
		parts[i].temp + 45.0f,
		solder.HighTemperature + 40.0f);
	return 0;
}

int OmniMetallurgyWoodUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim))
		return 0;
	if (parts[i].type != PT_WOOD
		|| parts[i].temp < 650.0f
		|| parts[i].temp >= 873.0f
		|| HasOxygen(x, y, parts, pmap)
		|| !HasNeighbour(x, y, PT_CRUC, parts, pmap))
	{
		parts[i].tmp3 = 0;
		return 0;
	}
	if (++parts[i].tmp3 < 60)
	{
		return 0;
	}
	sim->part_change_type(i, x, y, PT_CHRC);
	parts[i].temp = std::max(parts[i].temp, 650.0f);
	parts[i].tmp3 = 0;
	return 1;
}

int OmniMetallurgyCoalUpdate(UPDATE_FUNC_ARGS)
{
	if (!MetallurgyModuleEnabled(sim))
		return 0;
	if (parts[i].type != PT_COAL
		|| parts[i].temp < 950.0f
		|| HasOxygen(x, y, parts, pmap)
		|| !HasNeighbour(x, y, PT_CRUC, parts, pmap))
	{
		parts[i].tmp3 = 0;
		return 0;
	}
	if (++parts[i].tmp3 < 90)
	{
		return 0;
	}
	sim->part_change_type(i, x, y, PT_COKE);
	parts[i].life = 0;
	parts[i].tmp = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp3 = 0;
	return 1;
}
