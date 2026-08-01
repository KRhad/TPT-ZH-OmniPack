#include "OmniChemistry.h"

#include "ElementCommon.h"
#include "OmniModuleRuntime.h"
#include "OmniOrganics.h"

#include <algorithm>
#include <initializer_list>

namespace
{
constexpr int ChemistryReactionsPerFrame = 1536;
thread_local OmniModuleRuntimeCache chemistryRuntimeCache;

bool ChemistryModuleEnabled(Simulation *sim)
{
	return OmniModuleRuntimeEnabled(
		chemistryRuntimeCache, sim, sim->currentTick, "Omni.Modules.Chemistry");
}

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

Slot FindLocalOneOf(
	int x,
	int y,
	std::initializer_list<int> types,
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
			if (!packed)
				continue;
			auto index = ID(packed);
			auto type = TYP(packed);
			if (index == excludedIndex || IsTouched(index, parts, sim)
				|| std::find(types.begin(), types.end(), type) == types.end())
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

bool HasHotLocal(
	int x,
	int y,
	int type,
	float minimumTemperature,
	Parts &parts,
	int pmap[YRES][XRES])
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			auto packed = pmap[y + ry][x + rx];
			if (packed && TYP(packed) == type && parts[ID(packed)].temp >= minimumTemperature)
				return true;
		}
	}
	return false;
}

void Convert(Simulation *sim, Slot slot, int type, Parts &parts, float temperature)
{
	sim->part_change_type(slot.index, slot.x, slot.y, type);
	parts[slot.index].temp = temperature;
	parts[slot.index].life = 0;
	parts[slot.index].ctype = 0;
}

bool PeroxidePathogenTreatment(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PERO || IsTouched(i, parts, sim)
		|| parts[i].temp < 285.0f || parts[i].temp > 330.0f
		|| HasHotLocal(x, y, PT_CATA, 350.0f, parts, pmap))
		return false;
	auto pathogen = FindLocal(x, y, PT_PATH, i, parts, pmap, sim);
	if (pathogen.index < 0 || !ConsumeReactionBudget(sim))
		return false;

	auto temperature = (parts[i].temp + parts[pathogen.index].temp) * 0.5f;
	Convert(sim, Slot{ i, x, y }, PT_WATR, parts, temperature);
	Convert(sim, pathogen, PT_HUMS, parts, temperature);
	Touch(i, parts, sim);
	Touch(pathogen.index, parts, sim);
	return true;
}

bool SlagAcidLeaching(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA || IsTouched(i, parts, sim)
		|| parts[i].temp < 285.0f || parts[i].temp > 340.0f)
		return false;
	auto slag = FindLocal(x, y, PT_SLAG, i, parts, pmap, sim);
	auto acid = FindLocal(x, y, PT_ACID, slag.index, parts, pmap, sim);
	if (slag.index < 0 || acid.index < 0 || !ConsumeReactionBudget(sim))
		return false;

	auto temperature = (parts[i].temp + parts[slag.index].temp + parts[acid.index].temp) / 3.0f;
	Convert(sim, slag, PT_FLUX, parts, temperature);
	Convert(sim, acid, PT_WATR, parts, temperature);
	Touch(i, parts, sim);
	Touch(slag.index, parts, sim);
	Touch(acid.index, parts, sim);
	return true;
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
	if (hydrogen.index >= 0 && ConsumeReactionBudget(sim))
	{
		auto temperature = parts[i].temp;
		Convert(sim, { i, x, y }, PT_HCLA, parts, temperature);
		Convert(sim, hydrogen, PT_HCLA, parts, temperature);
		Touch(i, parts, sim);
		Touch(hydrogen.index, parts, sim);
		return true;
	}
	auto copper = FindLocal(x, y, PT_COPR, i, parts, pmap, sim);
	if (copper.index >= 0 && ConsumeReactionBudget(sim))
	{
		auto temperature = std::min(
			(parts[i].temp + parts[copper.index].temp) * 0.5f + 70.0f,
			MAX_TEMP);
		Convert(sim, { i, x, y }, PT_CUCL, parts, temperature);
		Convert(sim, copper, PT_CUCL, parts, temperature);
		Touch(i, parts, sim);
		Touch(copper.index, parts, sim);
		return true;
	}
	return false;
}

bool AmmoniaReaction(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_AMON || IsTouched(i, parts, sim))
		return false;
	auto acid = FindLocal(x, y, PT_ACID, i, parts, pmap, sim);
	auto self = Slot{ i, x, y };
	if (acid.index >= 0 && ConsumeReactionBudget(sim))
	{
		auto temperature = (parts[i].temp + parts[acid.index].temp) * 0.5f;
		Convert(sim, self, PT_FERT, parts, temperature);
		Convert(sim, acid, PT_FERT, parts, temperature);
		Touch(i, parts, sim);
		Touch(acid.index, parts, sim);
		return true;
	}

	auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
	if (water.index >= 0 && parts[i].temp >= 275.0f && parts[i].temp <= 315.0f
		&& ConsumeReactionBudget(sim))
	{
		auto temperature = (parts[i].temp + parts[water.index].temp) * 0.5f;
		Convert(sim, self, PT_AMWA, parts, temperature);
		Convert(sim, water, PT_AMWA, parts, temperature);
		Touch(i, parts, sim);
		Touch(water.index, parts, sim);
		return true;
	}
	return false;
}

int PeroxideOxidationProduct(int metal)
{
	switch (metal)
	{
	case PT_ALUM:
		return PT_ALOX;
	case PT_MAGN:
		return PT_MGOX;
	case PT_IRON:
		return PT_FEOX;
	case PT_COPR:
		return PT_CUOX;
	case PT_ZINC:
		return PT_ZNOX;
	case PT_CA:
		return PT_CAOH;
	case PT_BA:
		return PT_BAOH;
	case PT_TTAN:
		return PT_TIOX;
	case PT_URAN:
		return PT_UROX;
	default:
		return PT_NONE;
	}
}

bool PeroxideMetalOxidation(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PERO || IsTouched(i, parts, sim)
		|| parts[i].temp < 285.0f || parts[i].temp > 600.0f)
		return false;
	auto metal = FindLocalOneOf(
		x, y, { PT_ALUM, PT_MAGN, PT_IRON, PT_COPR, PT_ZINC, PT_CA, PT_BA,
			PT_TTAN, PT_URAN },
		i, parts, pmap, sim);
	if (metal.index < 0 || !ConsumeReactionBudget(sim))
		return false;
	auto product = PeroxideOxidationProduct(parts[metal.index].type);
	if (product == PT_NONE)
		return false;
	auto temperature = std::min(
		(parts[i].temp + parts[metal.index].temp) * 0.5f + 55.0f,
		MAX_TEMP);
	Convert(sim, { i, x, y }, PT_WATR, parts, temperature);
	Convert(sim, metal, product, parts, temperature);
	Touch(i, parts, sim);
	Touch(metal.index, parts, sim);
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

bool IsInorganicAcid(int type)
{
	return type == PT_HCLA || type == PT_SULA || type == PT_NITA
		|| type == PT_PHOA || type == PT_HYFA || type == PT_CARA;
}

bool IsInorganicBase(int type)
{
	return type == PT_NAOH || type == PT_KOH || type == PT_CAOH
		|| type == PT_BAOH || type == PT_AMWA;
}

int NeutralisationProduct(int acid, int base)
{
	if (acid == PT_HCLA)
	{
		if (base == PT_KOH) return PT_KCL;
		if (base == PT_CAOH) return PT_CACL;
		if (base == PT_AMWA) return PT_AMCL;
	}
	if (acid == PT_SULA && base == PT_NAOH) return PT_NASF;
	if (acid == PT_NITA)
	{
		if (base == PT_KOH) return PT_KNIT;
		if (base == PT_AMWA) return PT_AMNT;
	}
	if (acid == PT_PHOA)
	{
		if (base == PT_AMWA) return PT_FERT;
		if (base == PT_CAOH) return PT_CAPH;
	}
	if (acid == PT_CARA && base == PT_NAOH) return PT_NACO;
	return PT_SALT;
}

int AcidOxideProduct(int acid, int oxide)
{
	if (acid == PT_HCLA)
	{
		if (oxide == PT_CAOX) return PT_CACL;
		if (oxide == PT_FEOX) return PT_FECL;
	}
	if (acid == PT_SULA && oxide == PT_CUOX) return PT_CUSF;
	return PT_SALT;
}

int AcidMetalProduct(int acid, int metal)
{
	if (acid == PT_HCLA)
	{
		if (metal == PT_IRON) return PT_FECL;
		if (metal == PT_CA) return PT_CACL;
		if (metal == PT_ALUM) return PT_ALCL;
		if (metal == PT_MAGN) return PT_MGCL;
	}
	return PT_SALT;
}

float AcidMetalThreshold(int type)
{
	switch (type)
	{
	case PT_HCLA:
		return 295.0f;
	case PT_NITA:
		return 300.0f;
	case PT_HYFA:
		return 310.0f;
	case PT_PHOA:
		return 340.0f;
	case PT_SULA:
		return 370.0f;
	default:
		return MAX_TEMP;
	}
}

bool InorganicAcidNetwork(UPDATE_FUNC_ARGS)
{
	auto acidType = parts[i].type;
	if (!IsInorganicAcid(acidType) || IsTouched(i, parts, sim))
		return false;

	auto self = Slot{ i, x, y };
	if (acidType == PT_CARA && parts[i].temp >= 330.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto carbonDioxide = sim->create_part(-1, empty.x, empty.y, PT_CO2);
			if (carbonDioxide >= 0)
			{
				auto temperature = parts[i].temp;
				Convert(sim, self, PT_WATR, parts, temperature);
				parts[carbonDioxide].temp = temperature;
				Touch(i, parts, sim);
				Touch(carbonDioxide, parts, sim);
				return true;
			}
		}
	}
	if (acidType == PT_HYFA)
	{
		auto silica = FindLocalOneOf(
			x, y, { PT_GLAS, PT_QRTZ, PT_PQRT, PT_BSGL, PT_QGLS }, i, parts, pmap, sim);
		if (silica.index >= 0 && parts[i].temp >= 285.0f
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[silica.index].temp) * 0.5f;
			Convert(sim, self, PT_CAUS, parts, temperature + 45.0f);
			Convert(sim, silica, PT_DUST, parts, temperature);
			Touch(i, parts, sim);
			Touch(silica.index, parts, sim);
			return true;
		}
	}

	if (acidType == PT_SULA && parts[i].temp >= 450.0f)
	{
		auto copper = FindLocal(x, y, PT_COPR, i, parts, pmap, sim);
		if (copper.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[copper.index].temp) * 0.5f + 80.0f;
			Convert(sim, self, PT_CUSF, parts, temperature);
			Convert(sim, copper, PT_SODI, parts, temperature);
			Touch(i, parts, sim);
			Touch(copper.index, parts, sim);
			return true;
		}
	}

	if (acidType == PT_NITA && parts[i].temp >= 315.0f)
	{
		auto copper = FindLocal(x, y, PT_COPR, i, parts, pmap, sim);
		if (copper.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[copper.index].temp) * 0.5f + 65.0f;
			Convert(sim, self, PT_NODI, parts, temperature);
			Convert(sim, copper, PT_SALT, parts, temperature);
			Touch(i, parts, sim);
			Touch(copper.index, parts, sim);
			return true;
		}
	}

	if (acidType == PT_PHOA)
	{
		auto ammonia = FindLocal(x, y, PT_AMON, i, parts, pmap, sim);
		if (ammonia.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[ammonia.index].temp) * 0.5f;
			Convert(sim, self, PT_FERT, parts, temperature);
			Convert(sim, ammonia, PT_WATR, parts, temperature);
			Touch(i, parts, sim);
			Touch(ammonia.index, parts, sim);
			return true;
		}
	}

	if (acidType == PT_HCLA)
	{
		auto sulfide = FindLocalOneOf(
			x, y, { PT_FESF, PT_NASD }, i, parts, pmap, sim);
		if (sulfide.index >= 0 && parts[i].temp >= 295.0f
			&& ConsumeReactionBudget(sim))
		{
			auto product = parts[sulfide.index].type == PT_FESF
				? PT_FECL : PT_SALT;
			auto temperature = std::min(
				(parts[i].temp + parts[sulfide.index].temp) * 0.5f + 35.0f,
				MAX_TEMP);
			Convert(sim, self, PT_H2SG, parts, temperature);
			Convert(sim, sulfide, product, parts, temperature);
			Touch(i, parts, sim);
			Touch(sulfide.index, parts, sim);
			return true;
		}
	}

	auto oxide = FindLocalOneOf(
		x, y, { PT_CAOX, PT_ALOX, PT_MGOX, PT_FEOX, PT_CUOX, PT_ZNOX },
		i, parts, pmap, sim);
	if (oxide.index >= 0 && parts[i].temp >= 285.0f
		&& ConsumeReactionBudget(sim))
	{
		auto temperature = (parts[i].temp + parts[oxide.index].temp) * 0.5f + 35.0f;
		auto product = AcidOxideProduct(acidType, parts[oxide.index].type);
		Convert(sim, self, PT_WATR, parts, temperature);
		Convert(sim, oxide, product, parts, temperature);
		Touch(i, parts, sim);
		Touch(oxide.index, parts, sim);
		return true;
	}

	auto carbonate = FindLocalOneOf(
		x, y, { PT_CACO, PT_NABC, PT_NACO }, i, parts, pmap, sim);
	if (carbonate.index >= 0)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[carbonate.index].temp) * 0.5f;
			auto carbonDioxide = sim->create_part(-1, empty.x, empty.y, PT_CO2);
			if (carbonDioxide >= 0)
			{
				Convert(sim, self, PT_WATR, parts, temperature);
				Convert(sim, carbonate, PT_SALT, parts, temperature);
				parts[carbonDioxide].temp = temperature;
				Touch(i, parts, sim);
				Touch(carbonate.index, parts, sim);
				Touch(carbonDioxide, parts, sim);
				return true;
			}
		}
	}

	auto base = FindLocalOneOf(
		x, y, { PT_NAOH, PT_KOH, PT_CAOH, PT_BAOH, PT_AMWA },
		i, parts, pmap, sim);
	if (base.index >= 0 && ConsumeReactionBudget(sim))
	{
		auto product = NeutralisationProduct(acidType, parts[base.index].type);
		auto temperature = (parts[i].temp + parts[base.index].temp) * 0.5f + 25.0f;
		Convert(sim, self, PT_WATR, parts, temperature);
		Convert(sim, base, product, parts, temperature);
		Touch(i, parts, sim);
		Touch(base.index, parts, sim);
		return true;
	}

	auto metal = FindLocalOneOf(
		x, y, { PT_IRON, PT_ZINC, PT_ALUM, PT_MAGN, PT_CA }, i,
		parts, pmap, sim);
	auto threshold = AcidMetalThreshold(acidType);
	if (metal.index >= 0 && threshold < MAX_TEMP && parts[i].temp >= threshold
		&& ConsumeReactionBudget(sim))
	{
		auto temperature = (parts[i].temp + parts[metal.index].temp) * 0.5f + 55.0f;
		if (acidType == PT_NITA)
		{
			Convert(sim, self, PT_NODI, parts, temperature);
			Convert(sim, metal, PT_SALT, parts, temperature);
		}
		else
		{
			Convert(sim, self, PT_H2, parts, temperature);
			Convert(sim, metal, AcidMetalProduct(acidType, parts[metal.index].type),
				parts, temperature);
		}
		Touch(i, parts, sim);
		Touch(metal.index, parts, sim);
		return true;
	}
	return false;
}

bool InorganicBaseNetwork(UPDATE_FUNC_ARGS)
{
	auto type = parts[i].type;
	if ((!IsInorganicBase(type) && type != PT_CAOX) || IsTouched(i, parts, sim))
		return false;

	auto self = Slot{ i, x, y };
	if (type == PT_CAOX)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				(parts[i].temp + parts[water.index].temp) * 0.5f + 120.0f,
				MAX_TEMP);
			Convert(sim, self, PT_CAOH, parts, temperature);
			Convert(sim, water, PT_CAOH, parts, temperature);
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
		return false;
	}

	if (type == PT_CAOH && parts[i].temp >= 780.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto steam = sim->create_part(-1, empty.x, empty.y, PT_WTRV);
			if (steam >= 0)
			{
				auto temperature = parts[i].temp;
				Convert(sim, self, PT_CAOX, parts, temperature);
				parts[steam].temp = temperature;
				Touch(i, parts, sim);
				Touch(steam, parts, sim);
				return true;
			}
		}
	}

	if (type == PT_AMWA && parts[i].temp >= 371.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto steam = sim->create_part(-1, empty.x, empty.y, PT_WTRV);
			if (steam >= 0)
			{
				auto temperature = parts[i].temp;
				Convert(sim, self, PT_AMON, parts, temperature);
				parts[steam].temp = temperature;
				Touch(i, parts, sim);
				Touch(steam, parts, sim);
				return true;
			}
		}
	}

	if (type == PT_NAOH || type == PT_CAOH)
	{
		auto carbonDioxide = FindLocal(x, y, PT_CO2, i, parts, pmap, sim);
		if (carbonDioxide.index >= 0 && parts[i].temp >= 285.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto product = type == PT_NAOH
				? (parts[i].temp >= 360.0f ? PT_NACO : PT_NABC)
				: PT_CACO;
			auto temperature = (parts[i].temp + parts[carbonDioxide.index].temp) * 0.5f;
			Convert(sim, self, product, parts, temperature);
			Convert(sim, carbonDioxide, PT_WATR, parts, temperature);
			Touch(i, parts, sim);
			Touch(carbonDioxide.index, parts, sim);
			return true;
		}
	}

	if ((type == PT_NAOH || type == PT_KOH) && parts[i].temp >= 320.0f)
	{
		auto aluminium = FindLocal(x, y, PT_ALUM, i, parts, pmap, sim);
		if (aluminium.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[aluminium.index].temp) * 0.5f + 60.0f;
			Convert(sim, self, PT_SALT, parts, temperature);
			Convert(sim, aluminium, PT_H2, parts, temperature);
			Touch(i, parts, sim);
			Touch(aluminium.index, parts, sim);
			return true;
		}
	}
	return false;
}

bool InorganicSaltNetwork(UPDATE_FUNC_ARGS)
{
	auto type = parts[i].type;
	if (IsTouched(i, parts, sim))
		return false;
	auto self = Slot{ i, x, y };
	if (type == PT_CUSF)
	{
		auto iron = FindLocal(x, y, PT_IRON, i, parts, pmap, sim);
		if (iron.index >= 0 && parts[i].temp >= 285.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[iron.index].temp) * 0.5f;
			Convert(sim, self, PT_COPR, parts, temperature);
			Convert(sim, iron, PT_SALT, parts, temperature);
			Touch(i, parts, sim);
			Touch(iron.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_KCL || type == PT_CACL)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && parts[i].temp >= 275.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto heat = type == PT_CACL ? 75.0f : 20.0f;
			auto temperature = std::min(
				(parts[i].temp + parts[water.index].temp) * 0.5f + heat,
				MAX_TEMP);
			Convert(sim, self, PT_SLTW, parts, temperature);
			parts[water.index].temp = temperature;
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_FECL)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && parts[i].temp >= 285.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[water.index].temp) * 0.5f + 30.0f;
			Convert(sim, self, PT_FEOX, parts, temperature);
			Convert(sim, water, PT_HCLA, parts, temperature);
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_KNIT && parts[i].temp >= 550.0f)
	{
		auto fuel = FindLocalOneOf(
			x, y, { PT_COAL, PT_BCOL, PT_DUST }, i, parts, pmap, sim);
		if (fuel.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				(parts[i].temp + parts[fuel.index].temp) * 0.5f + 220.0f,
				MAX_TEMP);
			Convert(sim, self, PT_CO2, parts, temperature);
			Convert(sim, fuel, PT_FIRE, parts, temperature);
			parts[fuel.index].life = 20;
			Touch(i, parts, sim);
			Touch(fuel.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_AMNT && parts[i].temp >= 520.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto steam = sim->create_part(-1, empty.x, empty.y, PT_WTRV);
			if (steam >= 0)
			{
				auto temperature = std::min(parts[i].temp + 120.0f, MAX_TEMP);
				Convert(sim, self, PT_NODI, parts, temperature);
				parts[steam].temp = temperature;
				Touch(i, parts, sim);
				Touch(steam, parts, sim);
				return true;
			}
		}
	}
	else if (type == PT_KPER)
	{
		auto sulfide = FindLocal(x, y, PT_H2SG, i, parts, pmap, sim);
		if (sulfide.index >= 0 && parts[i].temp >= 285.0f
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				(parts[i].temp + parts[sulfide.index].temp) * 0.5f + 90.0f,
				MAX_TEMP);
			Convert(sim, self, PT_MN, parts, temperature);
			Convert(sim, sulfide, PT_SULA, parts, temperature);
			Touch(i, parts, sim);
			Touch(sulfide.index, parts, sim);
			return true;
		}
		if (parts[i].temp >= 480.0f)
		{
			auto fuel = FindLocalOneOf(
				x, y, { PT_COAL, PT_BCOL, PT_DUST, PT_WOOD },
				i, parts, pmap, sim);
			if (fuel.index >= 0 && ConsumeReactionBudget(sim))
			{
				auto temperature = std::min(
					(parts[i].temp + parts[fuel.index].temp) * 0.5f + 260.0f,
					MAX_TEMP);
				Convert(sim, self, PT_MN, parts, temperature);
				Convert(sim, fuel, PT_FIRE, parts, temperature);
				parts[fuel.index].life = 18;
				Touch(i, parts, sim);
				Touch(fuel.index, parts, sim);
				return true;
			}
		}
	}
	else if (type == PT_CAPH)
	{
		auto plant = FindLocal(x, y, PT_PLNT, i, parts, pmap, sim);
		auto water = FindLocal(x, y, PT_WATR, plant.index, parts, pmap, sim);
		if (plant.index >= 0 && water.index >= 0 && ConsumeReactionBudget(sim))
		{
			Convert(sim, self, PT_FERT, parts, parts[i].temp);
			Touch(i, parts, sim);
			Touch(plant.index, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_CACB)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				(parts[i].temp + parts[water.index].temp) * 0.5f + 90.0f,
				MAX_TEMP);
			Convert(sim, self, PT_CAOH, parts, temperature);
			Convert(sim, water, PT_ACTY, parts, temperature);
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_NAHY || type == PT_CAHY)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && ConsumeReactionBudget(sim))
		{
			auto product = type == PT_NAHY ? PT_NAOH : PT_CAOH;
			auto temperature = std::min(
				(parts[i].temp + parts[water.index].temp) * 0.5f + 140.0f,
				MAX_TEMP);
			Convert(sim, self, product, parts, temperature);
			Convert(sim, water, PT_H2, parts, temperature);
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_ALCL || type == PT_MGCL)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && parts[i].temp >= 285.0f
			&& ConsumeReactionBudget(sim))
		{
			auto oxide = type == PT_ALCL ? PT_ALOX : PT_MGOX;
			auto temperature = (parts[i].temp + parts[water.index].temp) * 0.5f + 25.0f;
			Convert(sim, self, oxide, parts, temperature);
			Convert(sim, water, PT_HCLA, parts, temperature);
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_CUCL)
	{
		auto iron = FindLocal(x, y, PT_IRON, i, parts, pmap, sim);
		if (iron.index >= 0 && parts[i].temp >= 285.0f
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = (parts[i].temp + parts[iron.index].temp) * 0.5f;
			Convert(sim, self, PT_COPR, parts, temperature);
			Convert(sim, iron, PT_FECL, parts, temperature);
			Touch(i, parts, sim);
			Touch(iron.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_AMCL && parts[i].temp >= 520.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto acid = sim->create_part(-1, empty.x, empty.y, PT_HCLA);
			if (acid >= 0)
			{
				auto temperature = parts[i].temp;
				Convert(sim, self, PT_AMON, parts, temperature);
				parts[acid].temp = temperature;
				Touch(i, parts, sim);
				Touch(acid, parts, sim);
				return true;
			}
		}
	}
	else if (type == PT_CACO && parts[i].temp >= 1100.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto carbonDioxide = sim->create_part(-1, empty.x, empty.y, PT_CO2);
			if (carbonDioxide >= 0)
			{
				auto temperature = parts[i].temp;
				Convert(sim, self, PT_CAOX, parts, temperature);
				parts[carbonDioxide].temp = temperature;
				Touch(i, parts, sim);
				Touch(carbonDioxide, parts, sim);
				return true;
			}
		}
	}
	else if (type == PT_NABC && parts[i].temp >= 430.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeReactionBudget(sim))
		{
			auto carbonDioxide = sim->create_part(-1, empty.x, empty.y, PT_CO2);
			if (carbonDioxide >= 0)
			{
				auto temperature = parts[i].temp;
				Convert(sim, self, PT_NACO, parts, temperature);
				parts[carbonDioxide].temp = temperature;
				Touch(i, parts, sim);
				Touch(carbonDioxide, parts, sim);
				return true;
			}
		}
	}
	return false;
}

bool InorganicGasNetwork(UPDATE_FUNC_ARGS)
{
	auto type = parts[i].type;
	if (IsTouched(i, parts, sim))
		return false;
	auto self = Slot{ i, x, y };
	if (type == PT_COMO)
	{
		auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
		if (oxygen.index >= 0
			&& std::max(parts[i].temp, parts[oxygen.index].temp) >= 600.0f
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				std::max(parts[i].temp, parts[oxygen.index].temp) + 180.0f,
				MAX_TEMP);
			Convert(sim, self, PT_CO2, parts, temperature);
			Convert(sim, oxygen, PT_FIRE, parts, temperature);
			parts[oxygen.index].life = 12;
			Touch(i, parts, sim);
			Touch(oxygen.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_SODI || type == PT_NODI)
	{
		auto peroxide = FindLocal(x, y, PT_PERO, i, parts, pmap, sim);
		if (peroxide.index >= 0 && parts[i].temp >= 285.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto product = type == PT_SODI ? PT_SULA : PT_NITA;
			auto temperature = (parts[i].temp + parts[peroxide.index].temp) * 0.5f + 20.0f;
			Convert(sim, self, product, parts, temperature);
			Convert(sim, peroxide, PT_WATR, parts, temperature);
			Touch(i, parts, sim);
			Touch(peroxide.index, parts, sim);
			return true;
		}
		if (type == PT_SODI)
		{
			auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
			if (oxygen.index >= 0
				&& std::max(parts[i].temp, parts[oxygen.index].temp) >= 650.0f
				&& ConsumeReactionBudget(sim))
			{
				auto temperature = std::min(
					std::max(parts[i].temp, parts[oxygen.index].temp) + 80.0f,
					MAX_TEMP);
				Convert(sim, self, PT_SUTR, parts, temperature);
				Convert(sim, oxygen, PT_SUTR, parts, temperature);
				Touch(i, parts, sim);
				Touch(oxygen.index, parts, sim);
				return true;
			}
		}
	}
	else if (type == PT_H2SG)
	{
		auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
		if (oxygen.index >= 0
			&& std::max(parts[i].temp, parts[oxygen.index].temp) >= 450.0f
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				std::max(parts[i].temp, parts[oxygen.index].temp) + 140.0f,
				MAX_TEMP);
			Convert(sim, self, PT_SODI, parts, temperature);
			Convert(sim, oxygen, PT_WATR, parts, temperature);
			Touch(i, parts, sim);
			Touch(oxygen.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_SUTR)
	{
		auto water = FindLocal(x, y, PT_WATR, i, parts, pmap, sim);
		if (water.index >= 0 && parts[i].temp >= 285.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				(parts[i].temp + parts[water.index].temp) * 0.5f + 70.0f,
				MAX_TEMP);
			Convert(sim, self, PT_SULA, parts, temperature);
			Convert(sim, water, PT_SULA, parts, temperature);
			Touch(i, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_NIMO)
	{
		auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
		if (oxygen.index >= 0
			&& std::max(parts[i].temp, parts[oxygen.index].temp) >= 400.0f
			&& ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				std::max(parts[i].temp, parts[oxygen.index].temp) + 60.0f,
				MAX_TEMP);
			Convert(sim, self, PT_NODI, parts, temperature);
			Convert(sim, oxygen, PT_NODI, parts, temperature);
			Touch(i, parts, sim);
			Touch(oxygen.index, parts, sim);
			return true;
		}
	}
	else if (type == PT_HYCN)
	{
		auto peroxide = FindLocal(x, y, PT_PERO, i, parts, pmap, sim);
		if (peroxide.index >= 0 && parts[i].temp >= 285.0f
			&& parts[i].temp <= 500.0f && ConsumeReactionBudget(sim))
		{
			auto temperature = std::min(
				(parts[i].temp + parts[peroxide.index].temp) * 0.5f + 55.0f,
				MAX_TEMP);
			Convert(sim, self, PT_CO2, parts, temperature);
			Convert(sim, peroxide, PT_NIMO, parts, temperature);
			Touch(i, parts, sim);
			Touch(peroxide.index, parts, sim);
			return true;
		}
	}
	return false;
}

int OxideReductionProduct(int oxide)
{
	switch (oxide)
	{
	case PT_FEOX:
		return PT_IRON;
	case PT_CUOX:
		return PT_COPR;
	case PT_ZNOX:
		return PT_ZINC;
	case PT_UROX:
		return PT_URAN;
	default:
		return PT_NONE;
	}
}

float OxideReductionThreshold(int oxide)
{
	switch (oxide)
	{
	case PT_CUOX:
		return 650.0f;
	case PT_FEOX:
		return 850.0f;
	case PT_ZNOX:
		return 950.0f;
	case PT_UROX:
		return 1200.0f;
	default:
		return MAX_TEMP;
	}
}

bool InorganicOxideNetwork(UPDATE_FUNC_ARGS)
{
	auto oxideType = parts[i].type;
	auto product = OxideReductionProduct(oxideType);
	if (product == PT_NONE || IsTouched(i, parts, sim))
		return false;
	auto reducer = FindLocalOneOf(
		x, y, { PT_COMO, PT_H2 }, i, parts, pmap, sim);
	if (reducer.index < 0
		|| std::max(parts[i].temp, parts[reducer.index].temp)
			< OxideReductionThreshold(oxideType)
		|| !ConsumeReactionBudget(sim))
	{
		return false;
	}
	auto reducerType = parts[reducer.index].type;
	auto temperature = std::min(
		std::max(parts[i].temp, parts[reducer.index].temp) + 70.0f,
		MAX_TEMP);
	Convert(sim, { i, x, y }, product, parts, temperature);
	Convert(sim, reducer, reducerType == PT_COMO ? PT_CO2 : PT_WATR,
		parts, temperature);
	Touch(i, parts, sim);
	Touch(reducer.index, parts, sim);
	return true;
}

bool InorganicCeramicNetwork(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SICB || IsTouched(i, parts, sim))
		return false;
	auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
	if (oxygen.index < 0
		|| std::max(parts[i].temp, parts[oxygen.index].temp) < 1600.0f
		|| !ConsumeReactionBudget(sim))
	{
		return false;
	}
	auto temperature = std::min(
		std::max(parts[i].temp, parts[oxygen.index].temp) + 100.0f,
		MAX_TEMP);
	Convert(sim, { i, x, y }, PT_QRTZ, parts, temperature);
	Convert(sim, oxygen, PT_CO2, parts, temperature);
	Touch(i, parts, sim);
	Touch(oxygen.index, parts, sim);
	return true;
}

bool CatalyticPairSynthesis(
	UPDATE_FUNC_ARGS,
	int firstType,
	int secondType,
	int productType,
	float minimumTemperature,
	float maximumTemperature,
	float minimumPressure)
{
	if (parts[i].temp < minimumTemperature || parts[i].temp > maximumTemperature
		|| sim->pv[y / CELL][x / CELL] < minimumPressure)
	{
		return false;
	}
	auto first = FindLocal(x, y, firstType, i, parts, pmap, sim);
	auto second = FindLocal(x, y, secondType, first.index, parts, pmap, sim);
	if (first.index < 0 || second.index < 0 || !ConsumeReactionBudget(sim))
		return false;
	auto temperature = parts[i].temp;
	Convert(sim, first, productType, parts, temperature);
	Convert(sim, second, productType, parts, temperature);
	Touch(first.index, parts, sim);
	Touch(second.index, parts, sim);
	Touch(i, parts, sim);
	return true;
}
}

bool OmniChemistryModuleEnabled(Simulation *sim)
{
	return ChemistryModuleEnabled(sim);
}

bool OmniConsumeChemistryEvent(Simulation *sim)
{
	return ConsumeReactionBudget(sim);
}

int OmniChemistryElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!ChemistryModuleEnabled(sim))
		return 0;
	if (OmniOrganicElementUpdate(UPDATE_FUNC_SUBCALL_ARGS)
		|| PeroxidePathogenTreatment(UPDATE_FUNC_SUBCALL_ARGS)
		|| SlagAcidLeaching(UPDATE_FUNC_SUBCALL_ARGS)
		|| CatalyticCracking(UPDATE_FUNC_SUBCALL_ARGS)
		|| CatalyticPeroxideDecomposition(UPDATE_FUNC_SUBCALL_ARGS)
		|| PeroxideMetalOxidation(UPDATE_FUNC_SUBCALL_ARGS)
		|| ChlorineReaction(UPDATE_FUNC_SUBCALL_ARGS)
		|| AmmoniaReaction(UPDATE_FUNC_SUBCALL_ARGS)
		|| FertilizerUse(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return 0;
}

int OmniInorganicElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!ChemistryModuleEnabled(sim))
		return 0;
	if (InorganicAcidNetwork(UPDATE_FUNC_SUBCALL_ARGS)
		|| InorganicBaseNetwork(UPDATE_FUNC_SUBCALL_ARGS)
		|| InorganicSaltNetwork(UPDATE_FUNC_SUBCALL_ARGS)
		|| InorganicOxideNetwork(UPDATE_FUNC_SUBCALL_ARGS)
		|| InorganicCeramicNetwork(UPDATE_FUNC_SUBCALL_ARGS)
		|| InorganicGasNetwork(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return 0;
}

int OmniChemistryYeastUpdate(UPDATE_FUNC_ARGS)
{
	if (!ChemistryModuleEnabled(sim))
		return 0;
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
	if (!ChemistryModuleEnabled(sim))
		return 0;
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
	auto sulfur = FindLocal(x, y, PT_S, i, parts, pmap, sim);
	auto sulfideHydrogen = FindLocal(x, y, PT_H2, sulfur.index, parts, pmap, sim);
	if (sulfur.index >= 0 && sulfideHydrogen.index >= 0
		&& parts[i].temp >= 450.0f && parts[i].temp <= 800.0f
		&& ConsumeReactionBudget(sim))
	{
		auto temperature = parts[i].temp;
		Convert(sim, sulfur, PT_H2SG, parts, temperature);
		Convert(sim, sulfideHydrogen, PT_H2SG, parts, temperature);
		Touch(sulfur.index, parts, sim);
		Touch(sulfideHydrogen.index, parts, sim);
		Touch(i, parts, sim);
		return 1;
	}
	if (CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_N, PT_O2, PT_NIMO,
			1000.0f, 1800.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_IRON, PT_S, PT_FESF,
			700.0f, 1300.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_NA, PT_S, PT_NASD,
			550.0f, 900.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_CAOX, PT_COAL, PT_CACB,
			1200.0f, 2000.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_SLCN, PT_COAL, PT_SICB,
			1500.0f, 2500.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_B, PT_N, PT_BORN,
			1200.0f, 2200.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_SLCN, PT_N, PT_SINT,
			1400.0f, 2300.0f, 0.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_NA, PT_H2, PT_NAHY,
			350.0f, 600.0f, 2.0f)
		|| CatalyticPairSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, PT_CA, PT_H2, PT_CAHY,
			500.0f, 800.0f, 2.0f))
	{
		return 1;
	}
	auto carbonDioxide = FindLocal(x, y, PT_CO2, i, parts, pmap, sim);
	auto carbonicWater = FindLocal(
		x, y, PT_WATR, carbonDioxide.index, parts, pmap, sim);
	if (carbonDioxide.index >= 0 && carbonicWater.index >= 0
		&& parts[i].temp >= 285.0f && parts[i].temp <= 325.0f
		&& ConsumeReactionBudget(sim))
	{
		auto temperature = parts[i].temp;
		Convert(sim, carbonDioxide, PT_CARA, parts, temperature);
		Convert(sim, carbonicWater, PT_CARA, parts, temperature);
		Touch(carbonDioxide.index, parts, sim);
		Touch(carbonicWater.index, parts, sim);
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
