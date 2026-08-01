#include "OmniIsotopes.h"

#include "ElementCommon.h"
#include "OmniNuclear.h"

#include <algorithm>
#include <array>

namespace
{
constexpr int MoltenIsotopeMarker = 0x4F4D4953; // "OMIS"

struct IsotopeProperties
{
	int decayProduct;
	int radiationType;
	int captureProduct;
	int lifeMin;
	int lifeMax;
	float decayHeat;
	float captureHeat;
	float meltingPoint;
	float fissionHeat;
	bool fissile;
	bool moderatorRequired;
	unsigned int emissionColour;
};

struct Slot
{
	int index = -1;
	int x = -1;
	int y = -1;
};

bool ChangeType(
	Simulation *sim,
	int index,
	int x,
	int y,
	int type,
	Parts &parts,
	float temperature);

IsotopeProperties PropertiesFor(int type)
{
	switch (type)
	{
	case PT_H2IS:
		return { NT, NT, PT_H3IS, 0, 0, 0.0f, 60.0f, 0.0f, 0.0f,
			false, false, 0xFFB8E8FF };
	case PT_H3IS:
		return { PT_HE, PT_ELEC, NT, 1800, 3000, 45.0f, 0.0f, 0.0f, 0.0f,
			false, false, 0xFF90D8FF };
	case PT_C14I:
		return { PT_N, PT_ELEC, NT, 2200, 3600, 55.0f, 0.0f, 4000.0f, 0.0f,
			false, false, 0xFF909090 };
	case PT_CO60:
		return { PT_NICL, PT_PHOT, NT, 1000, 1800, 180.0f, 0.0f, 1768.0f, 0.0f,
			false, false, 0xFF70A0FF };
	case PT_SR90:
		return { PT_Y, PT_ELEC, NT, 1500, 2400, 130.0f, 0.0f, 1050.0f, 0.0f,
			false, false, 0xFFFFC060 };
	case PT_I131:
		return { PT_XE, PT_PHOT, NT, 240, 480, 220.0f, 0.0f, 386.85f, 0.0f,
			false, false, 0xFFC070FF };
	case PT_CS37:
		return { PT_BA, PT_PHOT, NT, 1600, 2600, 200.0f, 0.0f, 301.59f, 0.0f,
			false, false, 0xFFFFD070 };
	case PT_TH32:
		return { PT_RA, PT_HE, PT_NFUL, 3000, 5000, 120.0f, 160.0f, 2023.0f, 0.0f,
			false, false, 0xFFD0D8C0 };
	case PT_U235:
		return { PT_TH, PT_HE, NT, 2800, 4500, 160.0f, 0.0f, 1405.0f, 900.0f,
			true, true, 0xFF90D060 };
	case PT_U238:
		return { PT_TH, PT_HE, PT_PU39, 3800, 6000, 110.0f, 240.0f, 1405.0f, 0.0f,
			false, false, 0xFF70B850 };
	case PT_PU39:
		return { PT_URAN, PT_HE, NT, 2200, 3600, 220.0f, 0.0f, 913.0f, 1100.0f,
			true, true, 0xFFFF9040 };
	case PT_AM41:
		return { PT_NP, PT_HE, NT, 800, 1400, 260.0f, 0.0f, 1449.0f, 0.0f,
			false, false, 0xFFC8D050 };
	case PT_CF52:
		return { PT_CM, PT_NEUT, NT, 300, 600, 420.0f, 0.0f, 1173.0f, 1250.0f,
			true, false, 0xFFFFE060 };
	default:
		return { NT, NT, NT, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f,
			false, false, 0 };
	}
}

bool IsIsotope(int type)
{
	return type == PT_H2IS || type == PT_H3IS || type == PT_C14I ||
		type == PT_CO60 || type == PT_SR90 || type == PT_I131 ||
		type == PT_CS37 || type == PT_TH32 || type == PT_U235 ||
		type == PT_U238 || type == PT_PU39 || type == PT_AM41 ||
		type == PT_CF52;
}

Slot FindLocalMatter(
	int x,
	int y,
	int type,
	int excluded,
	int pmap[YRES][XRES])
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != type || ID(packed) == excluded)
				continue;
			return { ID(packed), x + rx, y + ry };
		}
	}
	return {};
}

Slot FindLocalNeutron(
	int x,
	int y,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
					continue;
				return { ID(packed), x + rx, y + ry };
			}
		}
	}
	return {};
}

bool IgniteHydrogenIsotope(
	int i,
	int x,
	int y,
	int sourceType,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	if (sourceType != PT_H2IS && sourceType != PT_H3IS)
		return false;
	Slot igniter;
	for (int ry = -1; ry <= 1 && igniter.index < 0; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
				continue;
			auto type = TYP(packed);
			if (type != PT_FIRE && type != PT_PLSM && type != PT_LAVA)
				continue;
			igniter = { ID(packed), x + rx, y + ry };
			break;
		}
	}
	if (igniter.index < 0 || !OmniConsumeNuclearEvent(sim))
		return false;
	auto temperature = std::min(
		std::max({ parts[i].temp, parts[igniter.index].temp, 2473.15f }),
		MAX_TEMP);
	if (!ChangeType(sim, i, x, y, PT_FIRE, parts, temperature))
		return false;
	parts[i].life = sim->rng.between(180, 259);
	parts[i].tmp = 1;
	sim->pv[y / CELL][x / CELL] = std::min(
		sim->pv[y / CELL][x / CELL] + 0.25f, 12.0f);
	return true;
}

Slot FindEmptyMatter(int x, int y, int pmap[YRES][XRES])
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

void InitialiseState(Simulation *sim, Particle &particle, int type)
{
	auto properties = PropertiesFor(type);
	particle.ctype = PT_NONE;
	particle.tmp = 0;
	particle.tmp2 = 0;
	particle.tmp3 = 0;
	particle.tmp4 = 0;
	particle.life = properties.lifeMin > 0 ?
		sim->rng.between(properties.lifeMin, properties.lifeMax) : 0;
}

bool ChangeType(
	Simulation *sim,
	int index,
	int x,
	int y,
	int type,
	Parts &parts,
	float temperature)
{
	if (sim->part_change_type(index, x, y, type))
		return false;
	if (IsIsotope(type))
		InitialiseState(sim, parts[index], type);
	else
	{
		parts[index].life = 0;
		parts[index].ctype = PT_NONE;
		parts[index].tmp = 0;
		parts[index].tmp2 = 0;
		parts[index].tmp3 = 0;
		parts[index].tmp4 = 0;
	}
	parts[index].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	return true;
}

void EmitRadiation(
	Simulation *sim,
	int x,
	int y,
	int type,
	Parts &parts,
	int pmap[YRES][XRES],
	float temperature,
	unsigned int colour)
{
	if (type == NT)
		return;
	int radiation = -1;
	if (type == PT_HE)
	{
		auto empty = FindEmptyMatter(x, y, pmap);
		if (empty.x >= 0)
			radiation = sim->create_part(-1, empty.x, empty.y, type);
	}
	else
	{
		radiation = sim->create_part(-3, x, y, type);
	}
	if (radiation < 0)
		return;
	parts[radiation].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	if (type == PT_NEUT)
		parts[radiation].life = 24;
	else if (type == PT_PHOT)
	{
		parts[radiation].life = 18;
		parts[radiation].ctype = 0x0003FFF0;
	}
	else if (type == PT_ELEC)
		parts[radiation].life = 16;
	parts[radiation].dcolour = colour;
}

int TimerFor(const Particle &particle, bool molten)
{
	return molten ? particle.tmp3 : particle.life;
}

void SetTimer(Particle &particle, bool molten, int timer)
{
	if (molten)
		particle.tmp3 = timer;
	else
		particle.life = timer;
}

bool DecayIsotope(
	int i,
	int x,
	int y,
	int sourceType,
	bool molten,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	auto properties = PropertiesFor(sourceType);
	if (properties.decayProduct == NT || TimerFor(parts[i], molten) > 0)
		return false;
	if (!OmniConsumeNuclearEvent(sim))
	{
		SetTimer(parts[i], molten, 1);
		return false;
	}
	auto temperature = std::min(parts[i].temp + properties.decayHeat, MAX_TEMP);
	if (!ChangeType(sim, i, x, y, properties.decayProduct, parts, temperature))
		return false;
	EmitRadiation(sim, x, y, properties.radiationType, parts, pmap,
		temperature, properties.emissionColour);
	sim->pv[y / CELL][x / CELL] = std::min(
		sim->pv[y / CELL][x / CELL] + properties.decayHeat / 800.0f, 12.0f);
	return true;
}

int FissionProduct(Simulation *sim)
{
	constexpr std::array products{ PT_SR90, PT_I131, PT_CS37 };
	return products[sim->rng.between(0, int(products.size() - 1))];
}

bool ReactWithNeutron(
	int i,
	int x,
	int y,
	int sourceType,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	auto properties = PropertiesFor(sourceType);
	if (!properties.fissile && properties.captureProduct == NT)
		return false;
	auto neutron = FindLocalNeutron(x, y, pmap, sim);
	if (neutron.index < 0)
		return false;
	if (properties.fissile)
	{
		if (FindLocalMatter(x, y, PT_CROD, i, pmap).index >= 0)
			return false;
		if (properties.moderatorRequired &&
			FindLocalMatter(x, y, PT_MODR, i, pmap).index < 0)
			return false;
	}
	if (!OmniConsumeNuclearEvent(sim))
		return false;
	auto neutronTemperature = parts[neutron.index].temp;
	sim->kill_part(neutron.index);
	if (properties.fissile)
	{
		auto temperature = std::min(
			std::max(parts[i].temp, neutronTemperature) + properties.fissionHeat,
			MAX_TEMP);
		if (!ChangeType(sim, i, x, y, FissionProduct(sim), parts, temperature))
			return false;
		EmitRadiation(sim, x, y, PT_NEUT, parts, pmap,
			temperature, properties.emissionColour);
		sim->pv[y / CELL][x / CELL] = std::min(
			sim->pv[y / CELL][x / CELL] + 1.2f, 12.0f);
	}
	else
	{
		auto temperature = std::min(
			std::max(parts[i].temp, neutronTemperature) + properties.captureHeat,
			MAX_TEMP);
		if (!ChangeType(sim, i, x, y, properties.captureProduct, parts, temperature))
			return false;
	}
	return true;
}

bool MeltIsotope(
	int i,
	int x,
	int y,
	int sourceType,
	Parts &parts,
	Simulation *sim)
{
	auto properties = PropertiesFor(sourceType);
	if (properties.meltingPoint <= 0.0f || parts[i].temp < properties.meltingPoint)
		return false;
	auto temperature = parts[i].temp;
	auto timer = parts[i].life;
	if (sim->part_change_type(i, x, y, PT_LAVA))
		return false;
	parts[i].ctype = sourceType;
	parts[i].life = 240;
	parts[i].tmp = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp3 = timer;
	parts[i].tmp4 = MoltenIsotopeMarker;
	parts[i].temp = temperature;
	return true;
}

bool FreezeIsotope(
	int i,
	int x,
	int y,
	int sourceType,
	Parts &parts,
	Simulation *sim)
{
	auto properties = PropertiesFor(sourceType);
	if (properties.meltingPoint <= 0.0f || parts[i].temp >= properties.meltingPoint)
		return false;
	auto temperature = parts[i].temp;
	auto timer = parts[i].tmp3;
	if (sim->part_change_type(i, x, y, sourceType))
		return false;
	parts[i].ctype = PT_NONE;
	parts[i].life = timer;
	parts[i].tmp = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp3 = 0;
	parts[i].tmp4 = 0;
	parts[i].temp = temperature;
	return true;
}
}

int OmniIsotopeElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniNuclearModuleEnabled(sim) || !IsIsotope(parts[i].type))
		return 0;
	auto sourceType = parts[i].type;
	if (IgniteHydrogenIsotope(i, x, y, sourceType, parts, pmap, sim) ||
		ReactWithNeutron(i, x, y, sourceType, parts, pmap, sim) ||
		DecayIsotope(i, x, y, sourceType, false, parts, pmap, sim) ||
		MeltIsotope(i, x, y, sourceType, parts, sim))
		return 1;
	return 0;
}

int OmniIsotopeLavaUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniNuclearModuleEnabled(sim) || parts[i].type != PT_LAVA ||
		!IsIsotope(parts[i].ctype))
		return 0;
	auto sourceType = parts[i].ctype;
	auto properties = PropertiesFor(sourceType);
	if (parts[i].tmp4 != MoltenIsotopeMarker)
	{
		parts[i].tmp3 = properties.lifeMin > 0 ?
			sim->rng.between(properties.lifeMin, properties.lifeMax) : 0;
		parts[i].tmp4 = MoltenIsotopeMarker;
	}
	if (parts[i].tmp3 > 0)
		--parts[i].tmp3;
	if (ReactWithNeutron(i, x, y, sourceType, parts, pmap, sim) ||
		DecayIsotope(i, x, y, sourceType, true, parts, pmap, sim))
		return 1;
	FreezeIsotope(i, x, y, sourceType, parts, sim);
	// Recognised isotope lava bypasses the generic LAVA-water and stone path.
	return 1;
}

int OmniIsotopeSourceUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniNuclearModuleEnabled(sim) || parts[i].type != PT_COBT)
		return 0;
	auto neutron = FindLocalNeutron(x, y, pmap, sim);
	if (neutron.index < 0 || !OmniConsumeNuclearEvent(sim))
		return 0;
	auto temperature = std::min(
		std::max(parts[i].temp, parts[neutron.index].temp) + 100.0f, MAX_TEMP);
	sim->kill_part(neutron.index);
	if (!ChangeType(sim, i, x, y, PT_CO60, parts, temperature))
		return 0;
	return 1;
}

int OmniIsotopeGraphics(GRAPHICS_FUNC_ARGS)
{
	auto properties = PropertiesFor(cpart->type);
	if (!properties.emissionColour)
		return 0;
	auto pulse = cpart->life > 0 ? (cpart->life % 32) : 31;
	*colr = std::min(255, *colr + int((properties.emissionColour >> 16) & 0xFF) * pulse / 96);
	*colg = std::min(255, *colg + int((properties.emissionColour >> 8) & 0xFF) * pulse / 96);
	*colb = std::min(255, *colb + int(properties.emissionColour & 0xFF) * pulse / 96);
	*pixel_mode |= PMODE_GLOW;
	return 0;
}

void OmniIsotopeCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	InitialiseState(sim, sim->parts[i], t);
}

bool OmniIsotopeOwnsMoltenTransition(Particle const &particle)
{
	return particle.type == PT_LAVA && IsIsotope(particle.ctype);
}
