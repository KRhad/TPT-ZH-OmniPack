#include "OmniPeriodic.h"

#include "ElementCommon.h"

#include <algorithm>

namespace
{
constexpr int PeriodicEventsPerFrame = 1024;

struct ReactionBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = PeriodicEventsPerFrame;
};

thread_local ReactionBudget reactionBudget;

struct NobleGasProperties
{
	int dischargeChance;
	int glowLife;
	int dischargeHeat;
	int photonMask;
};

bool ConsumeEvent(Simulation *sim)
{
	if (reactionBudget.simulation != sim || reactionBudget.tick != sim->currentTick)
	{
		reactionBudget = { sim, sim->currentTick, PeriodicEventsPerFrame };
	}
	if (reactionBudget.remaining <= 0)
	{
		return false;
	}
	--reactionBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

NobleGasProperties PropertiesFor(int type)
{
	switch (type)
	{
	case PT_HE:
		return { 12, 7, 8, 0x03F00000 };
	case PT_NE:
		return { 6, 9, 18, 0x0007C000 };
	case PT_AR:
		return { 8, 8, 14, 0x001F8000 };
	case PT_KR:
		return { 7, 9, 20, 0x000FF000 };
	case PT_XE:
		return { 2, 12, 45, 0x03FC0000 };
	case PT_RN:
		return { 5, 10, 30, 0x0003FFF0 };
	case PT_OG:
		return { 3, 12, 65, 0x03F03F00 };
	default:
		return { 16, 6, 8, 0x03FFFFFF };
	}
}

bool IsDischargeSource(int packed)
{
	if (!packed)
	{
		return false;
	}
	auto type = TYP(packed);
	return type == PT_SPRK || type == PT_ELEC || type == PT_PLSM || type == PT_LIGH;
}

bool HasLocalDischarge(int x, int y, int pmap[YRES][XRES], Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			if (IsDischargeSource(pmap[y + ry][x + rx]) ||
				IsDischargeSource(sim->photons[y + ry][x + rx]))
			{
				return true;
			}
		}
	}
	return false;
}

bool EmitDischarge(UPDATE_FUNC_ARGS)
{
	if (parts[i].life > 0 || !HasLocalDischarge(x, y, pmap, sim))
	{
		return false;
	}
	auto properties = PropertiesFor(parts[i].type);
	if (!sim->rng.chance(1, properties.dischargeChance) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = properties.glowLife;
	parts[i].temp = std::min(parts[i].temp + float(properties.dischargeHeat), MAX_TEMP);
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = properties.photonMask;
		parts[photon].temp = parts[i].temp;
		parts[photon].life = 24;
	}
	return true;
}

bool ExchangeCryogenicHeat(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_HE || parts[i].temp >= 20.0f)
	{
		return false;
	}
	int start = sim->rng.between(0, 7);
	for (int offset = 0; offset < 8; ++offset)
	{
		int slot = (start + offset) % 8;
		int rx = (slot % 3) - 1;
		int ry = (slot / 3) - 1;
		if (slot >= 4)
		{
			++slot;
			rx = (slot % 3) - 1;
			ry = (slot / 3) - 1;
		}
		if (!InBounds(x + rx, y + ry))
		{
			continue;
		}
		auto packed = pmap[y + ry][x + rx];
		if (!packed)
		{
			continue;
		}
		auto neighbour = ID(packed);
		if (parts[neighbour].temp <= parts[i].temp + 1.0f || !ConsumeEvent(sim))
		{
			return false;
		}
		float transfer = std::min(4.0f, (parts[neighbour].temp - parts[i].temp) * 0.05f);
		parts[neighbour].temp -= transfer;
		parts[i].temp = std::min(parts[i].temp + transfer, 20.0f);
		return true;
	}
	return false;
}

bool DecayRadioactiveGas(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_RN && parts[i].type != PT_OG)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = parts[i].type == PT_RN
			? sim->rng.between(1200, 2400)
			: sim->rng.between(45, 120);
		return false;
	}
	--parts[i].tmp;
	if (parts[i].tmp > 0)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return false;
	}

	auto sourceType = parts[i].type;
	auto targetType = sourceType == PT_RN ? PT_POLO : PT_RN;
	auto temperature = std::min(parts[i].temp + (sourceType == PT_RN ? 120.0f : 420.0f), MAX_TEMP);
	sim->part_change_type(i, x, y, targetType);
	parts[i].temp = temperature;
	parts[i].life = 0;
	parts[i].ctype = 0;
	parts[i].tmp2 = 0;
	parts[i].tmp = targetType == PT_RN ? sim->rng.between(1200, 2400) : 0;

	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = sourceType == PT_RN ? 0x0003FFF0 : 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}
}

int OmniNobleGasUpdate(UPDATE_FUNC_ARGS)
{
	if (DecayRadioactiveGas(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	ExchangeCryogenicHeat(UPDATE_FUNC_SUBCALL_ARGS);
	EmitDischarge(UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

int OmniNobleGasGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 14, 150);
		*colr = std::min(*colr + intensity / 2, 255);
		*colg = std::min(*colg + intensity / 2, 255);
		*colb = std::min(*colb + intensity / 2, 255);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*firea = intensity;
		*pixel_mode |= PMODE_GLOW | PMODE_ADD | FIRE_ADD;
	}
	else if (cpart->type == PT_RN || cpart->type == PT_OG)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniNobleGasCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_RN)
	{
		sim->parts[i].tmp = sim->rng.between(1200, 2400);
	}
	else if (t == PT_OG)
	{
		sim->parts[i].tmp = sim->rng.between(45, 120);
	}
}
