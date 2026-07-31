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

struct AlkaliProperties
{
	int waterHeat;
	float pressure;
	int fireChance;
	float oxygenThreshold;
	float boilingPoint;
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

AlkaliProperties AlkaliPropertiesFor(int type)
{
	switch (type)
	{
	case PT_NA:
		return { 120, 0.8f, 4, 460.0f, 1156.0f };
	case PT_K:
		return { 220, 1.8f, 2, 410.0f, 1032.0f };
	case PT_CS:
		return { 380, 4.0f, 1, 360.0f, 944.0f };
	case PT_FR:
		return { 560, 7.0f, 1, 330.0f, 950.0f };
	default:
		return { 100, 0.5f, 6, 500.0f, 1200.0f };
	}
}

bool IsAlkaliMetal(int type)
{
	return type == PT_NA || type == PT_K || type == PT_CS || type == PT_FR;
}

bool IsWaterLike(int type)
{
	return type == PT_WATR || type == PT_DSTW || type == PT_SLTW ||
		type == PT_CBNW || type == PT_WTRV;
}

void ResetReactionProduct(Particle &particle, int type)
{
	particle.life = type == PT_CAUS ? 75 : 0;
	particle.ctype = 0;
	particle.tmp = 0;
	particle.tmp2 = 0;
}

void AddBoundedPressure(Simulation *sim, int x, int y, float amount)
{
	auto &pressure = sim->pv[y / CELL][x / CELL];
	pressure = std::min(pressure + amount, MAX_PRESSURE);
}

void EmitAlkaliFire(
	Simulation *sim, int x, int y, float temperature, int chance)
{
	if (chance > 1 && !sim->rng.chance(1, chance))
	{
		return;
	}
	int fire = sim->create_part(-3, x, y, PT_FIRE);
	if (fire >= 0)
	{
		sim->parts[fire].temp = temperature;
		sim->parts[fire].life = 16;
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

bool DecayFrancium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_FR || parts[i].type != PT_FR)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(180, 360);
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

	auto temperature = std::min(parts[i].temp + 360.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_POLO);
	ResetReactionProduct(parts[i], PT_POLO);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactAlkaliWithWaterOrAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkaliPropertiesFor(sourceType);
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
			auto neighbourType = TYP(packed);
			bool acid = neighbourType == PT_ACID;
			if (!acid && !IsWaterLike(neighbourType))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto neighbour = ID(packed);
			float heat = acid ? properties.waterHeat * 0.65f : properties.waterHeat;
			float temperature = std::min(
				std::max(parts[i].temp, parts[neighbour].temp) + heat,
				MAX_TEMP);
			int residue = acid ? PT_SALT : PT_CAUS;
			sim->part_change_type(i, x, y, residue);
			sim->part_change_type(neighbour, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], residue);
			ResetReactionProduct(parts[neighbour], PT_H2);
			parts[i].temp = temperature;
			parts[neighbour].temp = temperature;
			AddBoundedPressure(sim, x, y, acid ? properties.pressure * 0.5f : properties.pressure);
			EmitAlkaliFire(sim, x, y, temperature, acid ? properties.fireChance * 2 : properties.fireChance);
			return true;
		}
	}
	return false;
}

bool OxidiseHotAlkali(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkaliPropertiesFor(sourceType);
	if (parts[i].temp < properties.oxygenThreshold)
	{
		return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_O2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto oxygen = ID(packed);
			float temperature = std::min(
				parts[i].temp + properties.waterHeat + 240.0f,
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 24;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.75f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotAlkali(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkaliPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	float temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.5f);
	return true;
}

bool UpdateAlkali(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsAlkaliMetal(sourceType))
	{
		return false;
	}
	if (DecayFrancium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotAlkali(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactAlkaliWithWaterOrAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotAlkali(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
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

int OmniAlkaliMetalUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateAlkali(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenAlkaliUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateAlkali(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

void OmniAlkaliMetalCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_FR)
	{
		sim->parts[i].tmp = sim->rng.between(180, 360);
	}
}
