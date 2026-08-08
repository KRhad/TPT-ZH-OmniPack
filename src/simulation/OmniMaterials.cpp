#include "OmniMaterials.h"

#include "ElementCommon.h"
#include "OmniModuleRuntime.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace
{
constexpr int MaterialEventsPerFrame = 1536;
constexpr int GlassPressureMarker = 0x4F4D474C; // "OMGL"

thread_local OmniModuleRuntimeCache materialRuntimeCache;

struct EventBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = MaterialEventsPerFrame;
};

struct Slot
{
	int index = -1;
	int x = -1;
	int y = -1;
};

thread_local EventBudget eventBudget;

bool MaterialsModuleEnabled(Simulation *sim)
{
	return OmniModuleRuntimeEnabled(
		materialRuntimeCache, sim, sim->currentTick, "Omni.Modules.Metallurgy");
}

bool ConsumeEventBudget(Simulation *sim)
{
	if (eventBudget.simulation != sim || eventBudget.tick != sim->currentTick)
	{
		eventBudget.simulation = sim;
		eventBudget.tick = sim->currentTick;
		eventBudget.remaining = MaterialEventsPerFrame;
	}
	if (eventBudget.remaining <= 0)
	{
		return false;
	}
	--eventBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

Slot FindLocal(
	int x,
	int y,
	std::initializer_list<int> types,
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
			if (!packed)
			{
				continue;
			}
			auto type = TYP(packed);
			if (std::find(types.begin(), types.end(), type) == types.end())
			{
				continue;
			}
			auto index = ID(packed);
			if (ctype == PT_NONE || parts[index].ctype == ctype)
			{
				return { index, x + rx, y + ry };
			}
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
			{
				continue;
			}
			if (!pmap[y + ry][x + rx])
			{
				return { -1, x + rx, y + ry };
			}
		}
	}
	return {};
}

bool Convert(
	Simulation *sim,
	Slot slot,
	int type,
	Parts &parts,
	float temperature)
{
	if (slot.index < 0 || sim->part_change_type(slot.index, slot.x, slot.y, type))
	{
		return false;
	}
	auto &part = parts[slot.index];
	part.temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	part.life = 0;
	part.ctype = PT_NONE;
	part.tmp = 0;
	part.tmp2 = 0;
	part.tmp3 = 0;
	part.tmp4 = 0;
	part.dcolour = 0;
	return true;
}

bool ConvertMolten(
	Simulation *sim,
	Slot slot,
	int sourceType,
	Parts &parts,
	float temperature)
{
	if (!Convert(sim, slot, PT_LAVA, parts, temperature))
	{
		return false;
	}
	parts[slot.index].ctype = sourceType;
	parts[slot.index].life = 30;
	return true;
}

bool ReduceOre(
	int i,
	int x,
	int y,
	int reducerType,
	int productType,
	int byproductType,
	float minimumTemperature,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	auto reducer = FindLocal(x, y, { reducerType }, PT_NONE, parts, pmap);
	if (reducer.index < 0)
	{
		return false;
	}
	auto temperature = std::max(parts[i].temp, parts[reducer.index].temp);
	if (temperature < minimumTemperature || !ConsumeEventBudget(sim))
	{
		return false;
	}
	Slot self{ i, x, y };
	Convert(sim, self, productType, parts, temperature + 60.0f);
	Convert(sim, reducer, byproductType, parts, temperature + 30.0f);
	return true;
}

bool UpdateGypsum(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp == 0 && parts[i].temp >= 420.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && ConsumeEventBudget(sim))
		{
			auto steam = sim->create_part(-1, empty.x, empty.y, PT_WTRV);
			if (steam >= 0)
			{
				parts[steam].temp = std::max(parts[i].temp, 430.0f);
				parts[i].tmp = 1;
				parts[i].dcolour = 0xFFD8D0C5;
				parts[i].temp = std::max(parts[i].temp - 35.0f, 385.0f);
				return true;
			}
		}
	}
	if (parts[i].tmp == 1 && parts[i].temp < 360.0f)
	{
		auto water = FindLocal(x, y, { PT_WATR, PT_DSTW }, PT_NONE, parts, pmap);
		if (water.index >= 0 && ConsumeEventBudget(sim))
		{
			parts[i].temp = (parts[i].temp + parts[water.index].temp) * 0.5f;
			sim->kill_part(water.index);
			parts[i].tmp = 0;
			parts[i].dcolour = 0;
			return true;
		}
	}
	return false;
}

bool UpdateFeldspar(UPDATE_FUNC_ARGS)
{
	auto flux = FindLocal(x, y, { PT_FLUX }, PT_NONE, parts, pmap);
	if (flux.index < 0)
	{
		return false;
	}
	auto temperature = std::max(parts[i].temp, parts[flux.index].temp);
	if (temperature < 1400.0f || !ConsumeEventBudget(sim))
	{
		return false;
	}
	ConvertMolten(sim, { i, x, y }, PT_GLAS, parts, temperature + 180.0f);
	Convert(sim, flux, PT_SLAG, parts, temperature);
	return true;
}

bool UpdateCement(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp == 0)
	{
		auto water = FindLocal(x, y, { PT_WATR, PT_DSTW }, PT_NONE, parts, pmap);
		if (water.index >= 0 && parts[i].temp < 390.0f && ConsumeEventBudget(sim))
		{
			parts[i].temp = (parts[i].temp + parts[water.index].temp) * 0.5f;
			sim->kill_part(water.index);
			parts[i].tmp = 1;
			parts[i].life = 90;
			parts[i].dcolour = 0xFFB7B3A8;
			return true;
		}
	}
	else if (parts[i].tmp == 1 && parts[i].life <= 0 && ConsumeEventBudget(sim))
	{
		return Convert(sim, { i, x, y }, PT_CNCT, parts, parts[i].temp);
	}
	return false;
}

bool UpdateSpecialGlass(UPDATE_FUNC_ARGS)
{
	auto pressure = int(sim->pv[y / CELL][x / CELL] * 64.0f);
	if (parts[i].tmp4 != GlassPressureMarker)
	{
		parts[i].tmp3 = pressure;
		parts[i].tmp4 = GlassPressureMarker;
		return false;
	}
	auto threshold = parts[i].type == PT_BSGL ? 96 : 128;
	auto pressureDelta = pressure - parts[i].tmp3;
	parts[i].tmp3 = pressure;
	if (std::abs(pressureDelta) > threshold && ConsumeEventBudget(sim))
	{
		auto temperature = parts[i].temp;
		if (Convert(sim, { i, x, y }, PT_BGLA, parts, temperature))
		{
			return true;
		}
	}
	return false;
}

bool UpdateRefractoryBrick(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp < 1500.0f || !sim->rng.chance(1, 32))
	{
		return false;
	}
	auto water = FindLocal(x, y, { PT_WATR, PT_SLTW }, PT_NONE, parts, pmap);
	if (water.index < 0 || !ConsumeEventBudget(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	Convert(sim, { i, x, y }, PT_STNE, parts, temperature - 120.0f);
	Convert(sim, water, PT_WTRV, parts, std::max(temperature * 0.45f, 450.0f));
	return true;
}
}

bool IsGlassMaterialType(int type)
{
	return type == PT_GLAS || type == PT_BGLA || type == PT_BSGL || type == PT_QGLS;
}

int OmniMaterialsElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!MaterialsModuleEnabled(sim))
	{
		return 0;
	}
	switch (parts[i].type)
	{
	case PT_GYPS:
		return UpdateGypsum(UPDATE_FUNC_SUBCALL_ARGS) ? 1 : 0;
	case PT_BAUX:
		return ReduceOre(i, x, y, PT_NAOH, PT_ALOX, PT_SLAG, 650.0f,
			parts, pmap, sim) ? 1 : 0;
	case PT_CUOR:
		return ReduceOre(i, x, y, PT_COMO, PT_COPR, PT_CO2, 1050.0f,
			parts, pmap, sim) ? 1 : 0;
	case PT_ZNOR:
		return ReduceOre(i, x, y, PT_COMO, PT_ZINC, PT_CO2, 1200.0f,
			parts, pmap, sim) ? 1 : 0;
	case PT_PBOR:
		return ReduceOre(i, x, y, PT_COKE, PT_LEAD, PT_SODI, 1100.0f,
			parts, pmap, sim) ? 1 : 0;
	case PT_UORE:
		return ReduceOre(i, x, y, PT_PERO, PT_UROX, PT_WATR, 450.0f,
			parts, pmap, sim) ? 1 : 0;
	case PT_FELD:
		return UpdateFeldspar(UPDATE_FUNC_SUBCALL_ARGS) ? 1 : 0;
	case PT_CEMT:
		return UpdateCement(UPDATE_FUNC_SUBCALL_ARGS) ? 1 : 0;
	case PT_BSGL:
	case PT_QGLS:
		return UpdateSpecialGlass(UPDATE_FUNC_SUBCALL_ARGS) ? 1 : 0;
	case PT_RFBK:
		return UpdateRefractoryBrick(UPDATE_FUNC_SUBCALL_ARGS) ? 1 : 0;
	default:
		return 0;
	}
}

int OmniMaterialsLavaUpdate(UPDATE_FUNC_ARGS)
{
	if (!MaterialsModuleEnabled(sim) || parts[i].type != PT_LAVA
		|| parts[i].ctype != PT_QRTZ || parts[i].temp < 2000.0f)
	{
		return 0;
	}
	auto water = FindLocal(x, y, { PT_WATR, PT_SLTW }, PT_NONE, parts, pmap);
	if (water.index < 0 || !sim->rng.chance(1, 8) || !ConsumeEventBudget(sim))
	{
		return 0;
	}
	parts[i].ctype = PT_QGLS;
	parts[i].temp = 1850.0f;
	parts[i].life = 0;
	parts[i].tmp = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp3 = 0;
	parts[i].tmp4 = 0;
	Convert(sim, water, PT_WTRV, parts, 550.0f);
	return 1;
}

int OmniMaterialsSparkUpdate(UPDATE_FUNC_ARGS)
{
	if (!MaterialsModuleEnabled(sim) || parts[i].type != PT_SPRK
		|| parts[i].ctype != PT_NCRM)
	{
		return 0;
	}

	// More specific two-input recipes must precede single-powder sintering.
	auto brick = FindLocal(x, y, { PT_BRCK }, PT_NONE, parts, pmap);
	auto alumina = FindLocal(x, y, { PT_ALOX }, PT_NONE, parts, pmap);
	if (brick.index >= 0 && alumina.index >= 0)
	{
		auto temperature = std::max({
			parts[i].temp, parts[brick.index].temp, parts[alumina.index].temp });
		if (temperature >= 1300.0f && ConsumeEventBudget(sim))
		{
			Convert(sim, brick, PT_RFBK, parts, temperature);
			Convert(sim, alumina, PT_RFBK, parts, temperature);
			return 1;
		}
	}

	auto glass = FindLocal(x, y, { PT_LAVA }, PT_GLAS, parts, pmap);
	auto boron = FindLocal(x, y, { PT_B }, PT_NONE, parts, pmap);
	if (glass.index >= 0 && boron.index >= 0)
	{
		auto temperature = std::max({
			parts[i].temp, parts[glass.index].temp, parts[boron.index].temp });
		if (temperature >= 1900.0f && ConsumeEventBudget(sim))
		{
			ConvertMolten(sim, glass, PT_BSGL, parts, temperature);
			ConvertMolten(sim, boron, PT_BSGL, parts, temperature);
			return 1;
		}
	}

	if (alumina.index >= 0)
	{
		auto temperature = std::max(parts[i].temp, parts[alumina.index].temp);
		if (temperature >= 1700.0f && ConsumeEventBudget(sim))
		{
			Convert(sim, alumina, PT_ALCR, parts, temperature);
			return 1;
		}
	}
	return 0;
}
