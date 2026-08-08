#include "OmniEnvironment.h"

#include "ElementCommon.h"
#include "OmniBiology.h"
#include "OmniGasGraphics.h"
#include "prefs/GlobalPrefs.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace
{
struct Slot
{
	int index = -1;
	int x = -1;
	int y = -1;
};

bool IsTouched(int index, Parts &parts, Simulation *sim)
{
	return index >= 0 && parts[index].tmp3 == sim->currentTick + 1;
}

void Touch(int index, Parts &parts, Simulation *sim)
{
	if (index >= 0)
		parts[index].tmp3 = sim->currentTick + 1;
}

bool IsExcluded(int index, std::initializer_list<int> excluded)
{
	return std::find(excluded.begin(), excluded.end(), index) != excluded.end();
}

Slot FindLocal(
	int x,
	int y,
	std::initializer_list<int> types,
	std::initializer_list<int> excluded,
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
			if (!packed || std::find(types.begin(), types.end(), TYP(packed)) == types.end())
				continue;
			auto index = ID(packed);
			if (IsExcluded(index, excluded) || IsTouched(index, parts, sim))
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
			if ((rx || ry) && InBounds(x + rx, y + ry) && !pmap[y + ry][x + rx])
				return { -1, x + rx, y + ry };
		}
	}
	return {};
}

bool InGrowthWindow(Particle const &particle)
{
	return particle.temp >= 285.0f && particle.temp <= 315.0f;
}

bool SimplifiedBiology()
{
	return GlobalPrefs::Ref().Get("Omni.Simulation.SimplifiedBiology", false);
}

void InitialiseState(Simulation *sim, Particle &particle, int type)
{
	particle.life = 0;
	particle.ctype = PT_NONE;
	particle.tmp = 0;
	particle.tmp2 = 0;
	particle.tmp3 = 0;
	particle.tmp4 = 0;
	particle.dcolour = 0;
	switch (type)
	{
	case PT_RCON:
		particle.life = sim->rng.between(1200, 1800);
		break;
	case PT_BLOM:
		particle.life = 600;
		break;
	case PT_MOLD:
		particle.life = 480;
		break;
	case PT_AMAT:
		particle.tmp = 12;
		break;
	default:
		break;
	}
}

bool Convert(
	Simulation *sim,
	Slot slot,
	int type,
	Parts &parts,
	float temperature)
{
	if (slot.index < 0 || sim->part_change_type(slot.index, slot.x, slot.y, type))
		return false;
	if (OmniIsEnvironmentElement(type))
		InitialiseState(sim, parts[slot.index], type);
	else
	{
		parts[slot.index].life = 0;
		parts[slot.index].ctype = PT_NONE;
		parts[slot.index].tmp = 0;
		parts[slot.index].tmp2 = 0;
		parts[slot.index].tmp3 = 0;
		parts[slot.index].tmp4 = 0;
		parts[slot.index].dcolour = 0;
	}
	parts[slot.index].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	return true;
}

void EmitShortPhoton(
	Simulation *sim,
	int x,
	int y,
	Parts &parts,
	float temperature)
{
	auto photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon < 0)
		return;
	parts[photon].life = 12;
	parts[photon].ctype = 0x0003FFF0;
	parts[photon].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	parts[photon].dcolour = 0xFF98FF70;
}

bool UpdateSoil(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SOIL || IsTouched(i, parts, sim))
		return false;
	if (parts[i].temp >= 360.0f && parts[i].tmp > 0)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && OmniConsumeBiologyEvent(sim))
		{
			auto vapour = sim->create_part(-1, empty.x, empty.y, PT_WTRV);
			if (vapour >= 0)
			{
				parts[vapour].temp = std::max(parts[i].temp, 380.0f);
				--parts[i].tmp;
				Touch(i, parts, sim);
				return true;
			}
		}
	}
	if (parts[i].temp >= 275.0f && parts[i].temp <= 335.0f && parts[i].tmp < 8)
	{
		auto water = FindLocal(x, y, { PT_WATR, PT_DSTW }, { i }, parts, pmap, sim);
		if (water.index >= 0 && OmniConsumeBiologyEvent(sim))
		{
			sim->kill_part(water.index);
			++parts[i].tmp;
			Touch(i, parts, sim);
			return true;
		}
	}
	if (parts[i].tmp >= 2 && InGrowthWindow(parts[i]))
	{
		auto humus = FindLocal(x, y, { PT_HUMS }, { i }, parts, pmap, sim);
		auto nutrient = FindLocal(x, y, { PT_NUTR }, { i, humus.index }, parts, pmap, sim);
		if (humus.index >= 0 && nutrient.index >= 0 && OmniConsumeBiologyEvent(sim))
		{
			auto temperature = (parts[i].temp + parts[humus.index].temp + parts[nutrient.index].temp) / 3.0f;
			Convert(sim, humus, PT_SOIL, parts, temperature);
			Convert(sim, nutrient, PT_SOIL, parts, temperature);
			parts[i].tmp -= 2;
			Touch(i, parts, sim);
			Touch(humus.index, parts, sim);
			Touch(nutrient.index, parts, sim);
			return true;
		}
	}
	if (parts[i].tmp > 0)
	{
		auto toxin = FindLocal(x, y, { PT_TOXN }, { i }, parts, pmap, sim);
		if (toxin.index >= 0 && OmniConsumeBiologyEvent(sim))
		{
			Convert(sim, toxin, PT_SLUD, parts, parts[i].temp);
			--parts[i].tmp;
			Touch(i, parts, sim);
			Touch(toxin.index, parts, sim);
			return true;
		}
	}
	return false;
}

bool UpdateWastewater(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_WWTR || IsTouched(i, parts, sim))
		return false;
	auto biofilm = FindLocal(x, y, { PT_BIOF }, { i }, parts, pmap, sim);
	if (biofilm.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		auto temperature = (parts[i].temp + parts[biofilm.index].temp) * 0.5f;
		Convert(sim, { i, x, y }, PT_DSTW, parts, temperature);
		parts[biofilm.index].tmp += 1;
		if (parts[biofilm.index].tmp >= 8)
			Convert(sim, biofilm, PT_SLUD, parts, temperature);
		Touch(i, parts, sim);
		Touch(biofilm.index, parts, sim);
		return true;
	}
	if (!InGrowthWindow(parts[i]))
		return false;
	auto algae = FindLocal(x, y, { PT_ALGA }, { i }, parts, pmap, sim);
	auto nutrient = FindLocal(x, y, { PT_NUTR }, { i, algae.index }, parts, pmap, sim);
	if (algae.index < 0 || nutrient.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	auto temperature = (parts[i].temp + parts[algae.index].temp + parts[nutrient.index].temp) / 3.0f;
	Convert(sim, { i, x, y }, SimplifiedBiology() ? PT_SLUD : PT_BLOM, parts, temperature);
	Convert(sim, algae, SimplifiedBiology() ? PT_HUMS : PT_BLOM, parts, temperature);
	Convert(sim, nutrient, PT_HUMS, parts, temperature);
	Touch(i, parts, sim);
	Touch(algae.index, parts, sim);
	Touch(nutrient.index, parts, sim);
	return true;
}

bool UpdatePesticide(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PEST || IsTouched(i, parts, sim))
		return false;
	auto target = FindLocal(
		x, y, { PT_PATH, PT_MOLD, PT_BLOM, PT_ALGA }, { i }, parts, pmap, sim);
	if (target.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	auto product = parts[target.index].type == PT_BLOM ? PT_SLUD : PT_HUMS;
	auto temperature = (parts[i].temp + parts[target.index].temp) * 0.5f;
	Convert(sim, { i, x, y }, PT_TOXN, parts, temperature);
	Convert(sim, target, product, parts, temperature);
	Touch(i, parts, sim);
	Touch(target.index, parts, sim);
	return true;
}

bool UpdateHeavyMetal(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_HMET || IsTouched(i, parts, sim))
		return false;
	auto biofilm = FindLocal(x, y, { PT_BIOF }, { i }, parts, pmap, sim);
	if (biofilm.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		auto temperature = (parts[i].temp + parts[biofilm.index].temp) * 0.5f;
		Convert(sim, { i, x, y }, PT_SLUD, parts, temperature);
		parts[biofilm.index].tmp += 2;
		if (parts[biofilm.index].tmp >= 8)
			Convert(sim, biofilm, PT_SLUD, parts, temperature);
		Touch(i, parts, sim);
		Touch(biofilm.index, parts, sim);
		return true;
	}
	auto water = FindLocal(x, y, { PT_WATR, PT_DSTW }, { i }, parts, pmap, sim);
	if (water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, water, PT_WWTR, parts, (parts[i].temp + parts[water.index].temp) * 0.5f);
	Touch(i, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

bool UpdateRadioactiveContaminant(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_RCON || IsTouched(i, parts, sim))
		return false;
	if (parts[i].life > 0)
		--parts[i].life;
	if (parts[i].life <= 0)
	{
		if (!OmniConsumeBiologyEvent(sim))
		{
			parts[i].life = 1;
			return false;
		}
		auto temperature = std::min(parts[i].temp + 40.0f, MAX_TEMP);
		Convert(sim, { i, x, y }, PT_HMET, parts, temperature);
		EmitShortPhoton(sim, x, y, parts, temperature);
		sim->pv[y / CELL][x / CELL] = std::min(
			sim->pv[y / CELL][x / CELL] + 0.05f, 12.0f);
		Touch(i, parts, sim);
		return true;
	}
	if (parts[i].life % 32 != 0)
		return false;
	auto water = FindLocal(x, y, { PT_WATR, PT_DSTW }, { i }, parts, pmap, sim);
	if (water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, water, PT_WWTR, parts, parts[i].temp);
	Touch(i, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

bool UpdateMicroplastic(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_MPLS || IsTouched(i, parts, sim))
		return false;
	auto biofilm = FindLocal(x, y, { PT_BIOF }, { i }, parts, pmap, sim);
	if (biofilm.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		Convert(sim, { i, x, y }, PT_SLUD, parts, parts[i].temp);
		parts[biofilm.index].tmp += 1;
		if (parts[biofilm.index].tmp >= 8)
			Convert(sim, biofilm, PT_SLUD, parts, parts[i].temp);
		Touch(i, parts, sim);
		Touch(biofilm.index, parts, sim);
		return true;
	}
	if (parts[i].temp < 650.0f || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_SMOG, parts, parts[i].temp + 25.0f);
	Touch(i, parts, sim);
	return true;
}

bool UpdateOrganicWaste(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_OWST || IsTouched(i, parts, sim))
		return false;
	if (InGrowthWindow(parts[i]))
	{
		auto mycelium = FindLocal(x, y, { PT_MYCL }, { i }, parts, pmap, sim);
		auto water = FindLocal(x, y, { PT_WATR, PT_WWTR }, { i, mycelium.index }, parts, pmap, sim);
		if (mycelium.index >= 0 && water.index >= 0 && OmniConsumeBiologyEvent(sim))
		{
			auto temperature = (parts[i].temp + parts[water.index].temp) * 0.5f;
			Convert(sim, { i, x, y }, PT_HUMS, parts, temperature);
			Convert(sim, water, PT_NUTR, parts, temperature);
			Touch(i, parts, sim);
			Touch(mycelium.index, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	if (parts[i].temp < 600.0f || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_SMOG, parts, parts[i].temp + 40.0f);
	Touch(i, parts, sim);
	return true;
}

bool UpdateBloom(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_BLOM || IsTouched(i, parts, sim))
		return false;
	if (parts[i].life > 0)
		--parts[i].life;
	auto oxygen = FindLocal(x, y, { PT_O2 }, { i }, parts, pmap, sim);
	if (oxygen.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		Convert(sim, oxygen, PT_CO2, parts, parts[i].temp);
		++parts[i].tmp;
		if (parts[i].tmp >= 4)
			Convert(sim, { i, x, y }, PT_SLUD, parts, parts[i].temp);
		Touch(i, parts, sim);
		Touch(oxygen.index, parts, sim);
		return true;
	}
	if (parts[i].life > 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_SLUD, parts, parts[i].temp);
	Touch(i, parts, sim);
	return true;
}

bool UpdateMould(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_MOLD || IsTouched(i, parts, sim))
		return false;
	if (parts[i].life > 0)
		--parts[i].life;
	if (InGrowthWindow(parts[i]))
	{
		auto feed = FindLocal(
			x, y, { PT_OWST, PT_CELU, PT_WOOD }, { i }, parts, pmap, sim);
		auto water = FindLocal(
			x, y, { PT_WATR, PT_WWTR }, { i, feed.index }, parts, pmap, sim);
		if (feed.index >= 0 && water.index >= 0 && OmniConsumeBiologyEvent(sim))
		{
			Convert(sim, feed, SimplifiedBiology() ? PT_HUMS : PT_MOLD, parts, parts[i].temp);
			parts[i].life = 480;
			Touch(i, parts, sim);
			Touch(feed.index, parts, sim);
			Touch(water.index, parts, sim);
			return true;
		}
	}
	if ((parts[i].life > 0 && parts[i].temp <= 330.0f) || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_HUMS, parts, parts[i].temp);
	Touch(i, parts, sim);
	return true;
}

bool UpdateBlood(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_BLOD || IsTouched(i, parts, sim))
		return false;
	auto pathogen = FindLocal(x, y, { PT_PATH }, { i }, parts, pmap, sim);
	if (pathogen.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		Convert(sim, { i, x, y }, PT_TOXN, parts, parts[i].temp);
		Touch(i, parts, sim);
		Touch(pathogen.index, parts, sim);
		return true;
	}
	if (parts[i].tmp < 8)
	{
		auto oxygen = FindLocal(x, y, { PT_O2 }, { i }, parts, pmap, sim);
		if (oxygen.index >= 0 && OmniConsumeBiologyEvent(sim))
		{
			Convert(sim, oxygen, PT_CO2, parts, parts[i].temp);
			++parts[i].tmp;
			Touch(i, parts, sim);
			Touch(oxygen.index, parts, sim);
			return true;
		}
	}
	if (parts[i].temp >= 345.0f ||
		(std::abs(parts[i].vx) < 0.1f && std::abs(parts[i].vy) < 0.1f &&
		 std::abs(sim->pv[y / CELL][x / CELL]) < 1.0f))
	{
		parts[i].tmp2 = std::min(parts[i].tmp2 + 1, 180);
	}
	else
		parts[i].tmp2 = std::max(parts[i].tmp2 - 1, 0);
	if (parts[i].tmp2 < 180 || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_HUMS, parts, parts[i].temp);
	Touch(i, parts, sim);
	return true;
}

bool UpdateToxin(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_TOXN || IsTouched(i, parts, sim))
		return false;
	auto target = FindLocal(
		x, y, { PT_ALGA, PT_MYCL, PT_MOLD, PT_BLOM }, { i }, parts, pmap, sim);
	if (target.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	auto product = parts[target.index].type == PT_BLOM ? PT_SLUD : PT_HUMS;
	Convert(sim, { i, x, y }, PT_WWTR, parts, parts[i].temp);
	Convert(sim, target, product, parts, parts[i].temp);
	Touch(i, parts, sim);
	Touch(target.index, parts, sim);
	return true;
}

bool UpdateAntimicrobialMaterial(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_AMAT || IsTouched(i, parts, sim) || parts[i].tmp <= 0)
		return false;
	auto target = FindLocal(
		x, y, { PT_PATH, PT_MOLD, PT_BLOM }, { i }, parts, pmap, sim);
	if (target.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	auto product = parts[target.index].type == PT_BLOM ? PT_SLUD : PT_HUMS;
	Convert(sim, target, product, parts, parts[i].temp);
	--parts[i].tmp;
	if (parts[i].tmp <= 0)
		Convert(sim, { i, x, y }, PT_DUST, parts, parts[i].temp);
	Touch(i, parts, sim);
	Touch(target.index, parts, sim);
	return true;
}

bool UpdateSludge(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SLUD || IsTouched(i, parts, sim))
		return false;
	if (parts[i].temp >= 360.0f)
	{
		auto empty = FindEmpty(x, y, pmap);
		if (empty.x >= 0 && OmniConsumeBiologyEvent(sim))
		{
			auto vapour = sim->create_part(-1, empty.x, empty.y, PT_WTRV);
			if (vapour >= 0)
			{
				parts[vapour].temp = std::max(parts[i].temp, 380.0f);
				Convert(sim, { i, x, y }, PT_SOIL, parts, parts[i].temp - 20.0f);
				Touch(i, parts, sim);
				return true;
			}
		}
	}
	if (!InGrowthWindow(parts[i]))
		return false;
	auto mycelium = FindLocal(x, y, { PT_MYCL }, { i }, parts, pmap, sim);
	if (mycelium.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_HUMS, parts, parts[i].temp);
	Touch(i, parts, sim);
	Touch(mycelium.index, parts, sim);
	return true;
}

bool UpdateSmog(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SMOG || IsTouched(i, parts, sim))
		return false;
	auto vapour = FindLocal(x, y, { PT_WTRV }, { i }, parts, pmap, sim);
	if (vapour.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	auto temperature = (parts[i].temp + parts[vapour.index].temp) * 0.5f;
	Convert(sim, { i, x, y }, PT_ARAN, parts, temperature);
	Convert(sim, vapour, PT_ARAN, parts, temperature);
	Touch(i, parts, sim);
	Touch(vapour.index, parts, sim);
	return true;
}

bool UpdateAcidRain(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_ARAN || IsTouched(i, parts, sim))
		return false;
	auto lime = FindLocal(x, y, { PT_CAOH }, { i }, parts, pmap, sim);
	if (lime.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		auto temperature = (parts[i].temp + parts[lime.index].temp) * 0.5f;
		Convert(sim, { i, x, y }, PT_DSTW, parts, temperature + 12.0f);
		Convert(sim, lime, PT_SLUD, parts, temperature + 12.0f);
		Touch(i, parts, sim);
		Touch(lime.index, parts, sim);
		return true;
	}
	auto soil = FindLocal(x, y, { PT_SOIL }, { i }, parts, pmap, sim);
	if (soil.index >= 0 && OmniConsumeBiologyEvent(sim))
	{
		Convert(sim, { i, x, y }, PT_WWTR, parts, parts[i].temp);
		Convert(sim, soil, PT_SLUD, parts, parts[i].temp);
		Touch(i, parts, sim);
		Touch(soil.index, parts, sim);
		return true;
	}
	auto metal = FindLocal(
		x, y, { PT_METL, PT_IRON, PT_BMTL }, { i }, parts, pmap, sim);
	if (metal.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	Convert(sim, { i, x, y }, PT_WWTR, parts, parts[i].temp);
	Convert(sim, metal, PT_BRMT, parts, parts[i].temp);
	Touch(i, parts, sim);
	Touch(metal.index, parts, sim);
	return true;
}

bool UpdateDetergent(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_DETG || IsTouched(i, parts, sim))
		return false;
	auto oil = FindLocal(x, y, { PT_OIL }, { i }, parts, pmap, sim);
	auto water = FindLocal(
		x, y, { PT_WATR, PT_DSTW }, { i, oil.index }, parts, pmap, sim);
	if (oil.index < 0 || water.index < 0 || !OmniConsumeBiologyEvent(sim))
		return false;
	auto temperature = (parts[i].temp + parts[oil.index].temp + parts[water.index].temp) / 3.0f;
	Convert(sim, { i, x, y }, PT_WWTR, parts, temperature);
	Convert(sim, oil, PT_WWTR, parts, temperature);
	Convert(sim, water, PT_WWTR, parts, temperature);
	Touch(i, parts, sim);
	Touch(oil.index, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

void ConfigureMotion(
	Element &element,
	RGB colour,
	int state,
	int weight,
	int heatConduct,
	float heatCapacity,
	int hardness,
	unsigned int properties)
{
	element.Colour = colour;
	element.Advection = state == 2 ? 0.85f : (state == 1 ? 0.55f : 0.0f);
	element.AirDrag = state == 2 ? 0.01f * CFDS : (state == 1 ? 0.02f * CFDS : 0.0f);
	element.AirLoss = state == 2 ? 0.99f : (state == 1 ? 0.96f : 0.90f);
	element.Loss = state == 2 ? 0.35f : (state == 1 ? 0.82f : 0.0f);
	element.Collision = state == 0 ? 0.0f : -0.1f;
	element.Gravity = state == 2 ? -0.01f : (state == 1 ? 0.16f : 0.0f);
	element.Diffusion = state == 2 ? 0.55f : 0.0f;
	element.HotAir = state == 2 ? 0.0002f * CFDS : 0.0f;
	element.Falldown = state == 2 ? 0 : (state == 1 ? 1 : 0);
	element.Flammable = 0;
	element.Explosive = 0;
	element.Meltable = 0;
	element.Hardness = hardness;
	element.Weight = weight;
	element.HeatConduct = static_cast<unsigned char>(heatConduct);
	element.HeatCapacity = heatCapacity;
	element.Properties = properties;
	element.LowPressure = IPL;
	element.LowPressureTransition = NT;
	element.HighPressure = IPH;
	element.HighPressureTransition = NT;
	element.LowTemperature = ITL;
	element.LowTemperatureTransition = NT;
	element.HighTemperature = ITH;
	element.HighTemperatureTransition = NT;
	element.DefaultProperties.temp = 293.15f;
	element.Update = &OmniEnvironmentElementUpdate;
	element.Graphics = &OmniEnvironmentGraphics;
	element.Create = &OmniEnvironmentCreate;
}
}

bool OmniIsEnvironmentElement(int type)
{
	return type >= PT_SOIL && type <= PT_DETG;
}

void OmniConfigureEnvironmentElement(Element &element, int type)
{
	switch (type)
	{
	case PT_SOIL:
		ConfigureMotion(element, 0x8B642D_rgb, 1, 65, 45, 1.25f, 5,
			TYPE_PART | PROP_NEUTPASS);
		break;
	case PT_WWTR:
		ConfigureMotion(element, 0x536C5D_rgb, 1, 38, 72, 1.05f, 1,
			TYPE_LIQUID | PROP_CONDUCTS | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	case PT_PEST:
		ConfigureMotion(element, 0xB9C45A_rgb, 1, 34, 38, 0.92f, 2,
			TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	case PT_HMET:
		ConfigureMotion(element, 0x706A66_rgb, 1, 92, 110, 0.62f, 8,
			TYPE_PART | PROP_DEADLY | PROP_CONDUCTS);
		break;
	case PT_RCON:
		ConfigureMotion(element, 0x8FAE42_rgb, 1, 86, 42, 0.84f, 6,
			TYPE_PART | PROP_DEADLY | PROP_RADIOACTIVE | PROP_NEUTPASS);
		break;
	case PT_MPLS:
		ConfigureMotion(element, 0xD7C9C0_rgb, 1, 12, 8, 0.48f, 2,
			TYPE_PART | PROP_NEUTPASS);
		element.Flammable = 4;
		break;
	case PT_OWST:
		ConfigureMotion(element, 0x786047_rgb, 1, 36, 28, 1.20f, 3,
			TYPE_PART | PROP_NEUTPASS);
		element.Flammable = 8;
		break;
	case PT_BLOM:
		ConfigureMotion(element, 0x4B9A42_rgb, 1, 30, 54, 1.08f, 1,
			TYPE_LIQUID | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	case PT_MOLD:
		ConfigureMotion(element, 0x87966B_rgb, 1, 18, 24, 0.90f, 2,
			TYPE_PART | PROP_NEUTPASS);
		element.Flammable = 3;
		break;
	case PT_BLOD:
		ConfigureMotion(element, 0xA81418_rgb, 1, 33, 32, 1.02f, 2,
			TYPE_LIQUID | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	case PT_TOXN:
		ConfigureMotion(element, 0x80549B_rgb, 1, 42, 46, 0.88f, 2,
			TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	case PT_AMAT:
		ConfigureMotion(element, 0xB8C8CA_rgb, 0, 104, 18, 0.76f, 18,
			TYPE_SOLID | PROP_NEUTPASS);
		break;
	case PT_SLUD:
		ConfigureMotion(element, 0x4F4639_rgb, 1, 62, 30, 1.35f, 3,
			TYPE_LIQUID | PROP_NEUTPASS);
		element.Advection = 0.25f;
		element.Falldown = 2;
		break;
	case PT_SMOG:
		ConfigureMotion(element, 0x6C665F_rgb, 2, 3, 18, 0.74f, 0,
			TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS);
		break;
	case PT_ARAN:
		ConfigureMotion(element, 0x7B966D_rgb, 1, 36, 68, 1.02f, 1,
			TYPE_LIQUID | PROP_DEADLY | PROP_CONDUCTS | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	case PT_DETG:
		ConfigureMotion(element, 0x80BFD1_rgb, 1, 31, 48, 0.96f, 1,
			TYPE_LIQUID | PROP_NEUTPASS);
		element.Falldown = 2;
		break;
	default:
		break;
	}
}

void OmniEnvironmentCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	InitialiseState(sim, sim->parts[i], t);
}

int OmniEnvironmentElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniBiologyModuleEnabled(sim) || !OmniIsEnvironmentElement(parts[i].type))
		return 0;
	if (UpdateSoil(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateWastewater(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdatePesticide(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateHeavyMetal(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateRadioactiveContaminant(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateMicroplastic(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateOrganicWaste(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateBloom(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateMould(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateBlood(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateToxin(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateAntimicrobialMaterial(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateSludge(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateSmog(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateAcidRain(UPDATE_FUNC_SUBCALL_ARGS)
		|| UpdateDetergent(UPDATE_FUNC_SUBCALL_ARGS))
		return 1;
	return 0;
}

int OmniEnvironmentGraphics(GRAPHICS_FUNC_ARGS)
{
	switch (cpart->type)
	{
	case PT_SOIL:
		*colr = std::max(*colr - cpart->tmp * 5, 0);
		*colg = std::max(*colg - cpart->tmp * 4, 0);
		*colb = std::min(*colb + cpart->tmp * 2, 255);
		break;
	case PT_RCON:
		*pixel_mode |= PMODE_GLOW;
		*colg = std::min(*colg + 35, 255);
		break;
	case PT_BLOM:
	case PT_BLOD:
		*pixel_mode |= PMODE_BLUR;
		break;
	case PT_MOLD:
		*colr = std::clamp(*colr + (cpart->life % 7) - 3, 0, 255);
		*colg = std::clamp(*colg + (cpart->life % 11) - 5, 0, 255);
		break;
	case PT_SMOG:
		return OmniGasGraphics(GRAPHICS_FUNC_SUBCALL_ARGS);
	default:
		break;
	}
	return 0;
}
