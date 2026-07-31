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

struct AlkalineEarthProperties
{
	int waterHeat;
	float pressure;
	int fireChance;
	float waterThreshold;
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	unsigned int flameColour;
};

struct BoronGroupProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int flameColour;
};

struct CarbonGroupProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	int oxideProduct;
	unsigned int flameColour;
};

struct NitrogenGroupProperties
{
	float acidThreshold;
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	int oxideProduct;
	unsigned int flameColour;
};

struct OxygenGroupProperties
{
	float oxygenThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	int oxideProduct;
	unsigned int flameColour;
};

struct HalogenProperties
{
	float hydrogenThreshold;
	float metalThreshold;
	float disinfectionThreshold;
	float boilingPoint;
	int reactionHeat;
	float pressure;
	unsigned int vapourColour;
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

AlkalineEarthProperties AlkalineEarthPropertiesFor(int type)
{
	switch (type)
	{
	case PT_BE:
		return { 70, 0.25f, 0, 900.0f, 330.0f, 900.0f, 2742.0f, 0xFFE8F4FF };
	case PT_MAGN:
		return { 150, 0.6f, 8, 650.0f, 293.0f, MAX_TEMP, 1363.0f, 0xFFFFFFFF };
	case PT_CA:
		return { 170, 1.0f, 8, 273.0f, 273.0f, 650.0f, 1757.0f, 0xFFFF8A35 };
	case PT_SR:
		return { 250, 2.0f, 4, 273.0f, 273.0f, 580.0f, 1655.0f, 0xFFFF3030 };
	case PT_BA:
		return { 340, 3.5f, 2, 273.0f, 273.0f, 520.0f, 1500.0f, 0xFF66FF66 };
	case PT_RA:
		return { 430, 5.0f, 2, 273.0f, 273.0f, 480.0f, 1413.0f, 0xFF70FFB0 };
	default:
		return { 80, 0.25f, 0, MAX_TEMP, MAX_TEMP, MAX_TEMP, MAX_TEMP, 0 };
	}
}

BoronGroupProperties BoronGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_B:
		return { MAX_TEMP, 1000.0f, 4200.0f, 160, 0.3f, 0xFFFFC080 };
	case PT_ALUM:
		return { 360.0f, 1100.0f, 2743.0f, 220, 0.7f, 0xFFFFFFFF };
	case PT_GA:
		return { 320.0f, 700.0f, 2673.0f, 120, 0.4f, 0xFFA0C0FF };
	case PT_IN:
		return { 330.0f, 650.0f, 2345.0f, 130, 0.5f, 0xFF89A8FF };
	case PT_TL:
		return { 293.0f, 520.0f, 1746.0f, 180, 0.8f, 0xFF66CC66 };
	case PT_NH:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 500, 1.0f, 0xFFFF80C0 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

CarbonGroupProperties CarbonGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_SLCN:
		return { MAX_TEMP, 900.0f, 4000.0f, 180, 0.3f, PT_GLAS, 0xFFDDEEFF };
	case PT_GE:
		return { 360.0f, 720.0f, 3106.0f, 130, 0.4f, PT_GLAS, 0xFF9FB8FF };
	case PT_TIN:
		return { 330.0f, 750.0f, 2875.0f, 110, 0.35f, PT_SALT, 0xFFC8D8FF };
	case PT_LEAD:
		return { 360.0f, 650.0f, 2022.0f, 90, 0.45f, PT_SALT, 0xFFB0B8C0 };
	case PT_FL:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 550, 1.0f, PT_NONE, 0xFFFF70B0 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, PT_NONE, 0 };
	}
}

NitrogenGroupProperties NitrogenGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_N:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 20, 0.1f, PT_NONE, 0xFFB080FF };
	case PT_P:
		return { MAX_TEMP, 320.0f, 554.0f, 340, 0.75f, PT_DUST, 0xFFE8FFB0 };
	case PT_AS:
		return { 380.0f, 620.0f, 887.0f, 160, 0.45f, PT_DUST, 0xFFB8D8FF };
	case PT_SB:
		return { 350.0f, 680.0f, 1908.0f, 130, 0.4f, PT_SALT, 0xFFC8D0FF };
	case PT_BI:
		return { 390.0f, 720.0f, 1837.0f, 100, 0.35f, PT_SALT, 0xFFD8B0FF };
	case PT_MC:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 600, 1.0f, PT_NONE, 0xFFFF6090 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, PT_NONE, 0 };
	}
}

OxygenGroupProperties OxygenGroupPropertiesFor(int type)
{
	switch (type)
	{
	case PT_S:
		return { 390.0f, 718.0f, 300, 0.6f, PT_SMKE, 0xFFFFFF50 };
	case PT_SE:
		return { 620.0f, 958.0f, 170, 0.4f, PT_DUST, 0xFF80A0FF };
	case PT_TE:
		return { 780.0f, 1261.0f, 140, 0.35f, PT_GLAS, 0xFFD0E0FF };
	case PT_LV:
		return { MAX_TEMP, MAX_TEMP, 650, 1.0f, PT_NONE, 0xFFFF5080 };
	default:
		return { MAX_TEMP, MAX_TEMP, 0, 0.0f, PT_NONE, 0 };
	}
}

HalogenProperties HalogenPropertiesFor(int type)
{
	switch (type)
	{
	case PT_F:
		return { 293.0f, 273.0f, 273.0f, MAX_TEMP, 500, 1.4f, 0xFFDFF56A };
	case PT_CHLR:
		return { MAX_TEMP, 320.0f, 273.0f, MAX_TEMP, 300, 0.8f, 0xFF95C84B };
	case PT_BR:
		return { 380.0f, 360.0f, 290.0f, 332.0f, 220, 0.6f, 0xFF8B2F20 };
	case PT_I:
		return { 500.0f, 480.0f, 320.0f, 457.0f, 160, 0.4f, 0xFF7B3FA0 };
	case PT_AT:
		return { 520.0f, 520.0f, 340.0f, MAX_TEMP, 240, 0.5f, 0xFF4E4057 };
	case PT_TS:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, MAX_TEMP, 700, 1.0f, 0xFFFF4060 };
	default:
		return { MAX_TEMP, MAX_TEMP, MAX_TEMP, MAX_TEMP, 0, 0.0f, 0 };
	}
}

bool IsAlkaliMetal(int type)
{
	return type == PT_NA || type == PT_K || type == PT_CS || type == PT_FR;
}

bool IsAlkalineEarthMetal(int type)
{
	return type == PT_BE || type == PT_MAGN || type == PT_CA ||
		type == PT_SR || type == PT_BA || type == PT_RA;
}

bool IsBoronGroupElement(int type)
{
	return type == PT_B || type == PT_ALUM || type == PT_GA ||
		type == PT_IN || type == PT_TL || type == PT_NH;
}

bool IsReactiveCarbonGroupElement(int type)
{
	return type == PT_SLCN || type == PT_GE || type == PT_TIN ||
		type == PT_LEAD || type == PT_FL;
}

bool IsNitrogenGroupElement(int type)
{
	return type == PT_N || type == PT_P || type == PT_AS ||
		type == PT_SB || type == PT_BI || type == PT_MC;
}

bool IsOxygenGroupExtension(int type)
{
	return type == PT_S || type == PT_SE || type == PT_TE || type == PT_LV;
}

bool IsHalogenElement(int type)
{
	return type == PT_F || type == PT_CHLR || type == PT_BR ||
		type == PT_I || type == PT_AT || type == PT_TS;
}

bool IsHalogenReactiveMetal(int type)
{
	return type == PT_LITH || type == PT_NA || type == PT_K ||
		type == PT_RBDM || type == PT_CS || type == PT_MAGN ||
		type == PT_CA || type == PT_SR || type == PT_BA ||
		type == PT_ALUM || type == PT_COPR || type == PT_IRON ||
		type == PT_METL;
}

bool IsDisinfectionTarget(int type)
{
	return type == PT_PATH || type == PT_SPOR || type == PT_MYCL ||
		type == PT_BIOF;
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

void EmitPeriodicFire(
	Simulation *sim, int x, int y, float temperature, int chance,
	unsigned int colour = 0)
{
	if (chance <= 0)
	{
		return;
	}
	if (chance > 1 && !sim->rng.chance(1, chance))
	{
		return;
	}
	int fire = sim->create_part(-3, x, y, PT_FIRE);
	if (fire >= 0)
	{
		sim->parts[fire].temp = temperature;
		sim->parts[fire].life = 16;
		if (colour)
		{
			sim->parts[fire].dcolour = colour;
		}
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

bool HasLocalPhoton(int x, int y, Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = sim->photons[y + ry][x + rx];
			if (packed && TYP(packed) == PT_PHOT)
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
			EmitPeriodicFire(sim, x, y, temperature, acid ? properties.fireChance * 2 : properties.fireChance);
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

bool DecayRadium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_RA || parts[i].type != PT_RA)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(900, 1800);
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

	auto temperature = std::min(parts[i].temp + 240.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_RN);
	ResetReactionProduct(parts[i], PT_RN);
	parts[i].temp = temperature;
	parts[i].tmp = sim->rng.between(1200, 2400);
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactAlkalineEarthWithWaterOrAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkalineEarthPropertiesFor(sourceType);
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
			auto neighbour = ID(packed);
			float threshold = acid ? properties.acidThreshold : properties.waterThreshold;
			if (std::max(parts[i].temp, parts[neighbour].temp) < threshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			float heat = acid ? properties.waterHeat * 0.55f : properties.waterHeat;
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
			AddBoundedPressure(
				sim, x, y,
				acid ? properties.pressure * 0.4f : properties.pressure);
			EmitPeriodicFire(
				sim, x, y, temperature,
				acid ? properties.fireChance * 2 : properties.fireChance,
				properties.flameColour);
			return true;
		}
	}
	return false;
}

bool OxidiseHotAlkalineEarth(UPDATE_FUNC_ARGS, int sourceType)
{
	// Magnesium retains its existing metallurgy implementation, which produces
	// typed recoverable scrap and a bright finite flame above 800 K.
	if (sourceType == PT_MAGN)
	{
		return false;
	}
	auto properties = AlkalineEarthPropertiesFor(sourceType);
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
				parts[i].temp + properties.waterHeat + 180.0f,
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 24;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.6f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotAlkalineEarth(UPDATE_FUNC_ARGS, int sourceType)
{
	auto properties = AlkalineEarthPropertiesFor(sourceType);
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
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
	return true;
}

bool UpdateAlkalineEarth(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsAlkalineEarthMetal(sourceType))
	{
		return false;
	}
	if (DecayRadium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactAlkalineEarthWithWaterOrAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayNihonium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_NH || parts[i].type != PT_NH)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(90, 180);
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

	auto temperature = std::min(parts[i].temp + 500.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_POLO);
	ResetReactionProduct(parts[i], PT_POLO);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool CaptureBoronNeutron(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_B)
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
			for (auto packed : { pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed || TYP(packed) != PT_NEUT)
				{
					continue;
				}
				if (!ConsumeEvent(sim))
				{
					return false;
				}

				auto neutron = ID(packed);
				auto temperature = std::min(
					std::max(parts[i].temp, parts[neutron].temp) + 180.0f,
					MAX_TEMP);
				sim->part_change_type(i, x, y, PT_LITH);
				sim->part_change_type(neutron, x + rx, y + ry, PT_HE);
				ResetReactionProduct(parts[i], PT_LITH);
				ResetReactionProduct(parts[neutron], PT_HE);
				parts[i].temp = temperature;
				parts[neutron].temp = temperature;
				AddBoundedPressure(sim, x, y, 0.35f);
				return true;
			}
		}
	}
	return false;
}

bool EmbrittleAluminiumWithGallium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_GA || parts[i].temp < 302.91f)
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
			if (!packed)
			{
				continue;
			}
			auto neighbour = ID(packed);
			auto neighbourType = TYP(packed);
			if (neighbourType != PT_ALUM &&
				(neighbourType != PT_LAVA || parts[neighbour].ctype != PT_ALUM))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = parts[neighbour].temp;
			sim->part_change_type(neighbour, x + rx, y + ry, PT_MSCR);
			ResetReactionProduct(parts[neighbour], PT_MSCR);
			parts[neighbour].ctype = PT_ALUM;
			parts[neighbour].temp = temperature;
			parts[neighbour].tmp3 = 0;
			parts[neighbour].tmp4 = 0;
			parts[i].temp = std::min(parts[i].temp + 12.0f, MAX_TEMP);
			return true;
		}
	}
	return false;
}

bool ReactBoronGroupWithAcidOrCaustic(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_B || sourceType == PT_NH)
	{
		return false;
	}
	auto properties = BoronGroupPropertiesFor(sourceType);
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
			bool caustic = neighbourType == PT_CAUS &&
				(sourceType == PT_ALUM || sourceType == PT_GA);
			if (!acid && !caustic)
			{
				continue;
			}
			auto neighbour = ID(packed);
			if (std::max(parts[i].temp, parts[neighbour].temp) < properties.acidThreshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = std::min(
				std::max(parts[i].temp, parts[neighbour].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(neighbour, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[neighbour], PT_H2);
			parts[i].temp = temperature;
			parts[neighbour].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotBoronGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_NH)
	{
		return false;
	}
	auto properties = BoronGroupPropertiesFor(sourceType);
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
			auto oxide = sourceType == PT_B ? PT_GLAS : PT_SALT;
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, oxide);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], oxide);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.4f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotBoronGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_NH)
	{
		return false;
	}
	auto properties = BoronGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool UpdateBoronGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsBoronGroupElement(sourceType))
	{
		return false;
	}
	if (DecayNihonium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (CaptureBoronNeutron(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (EmbrittleAluminiumWithGallium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactBoronGroupWithAcidOrCaustic(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return OxidiseHotBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayFlerovium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_FL || parts[i].type != PT_FL)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(60, 130);
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

	auto temperature = std::min(parts[i].temp + 550.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_POLO);
	ResetReactionProduct(parts[i], PT_POLO);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool EmbrittleColdTin(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_TIN || parts[i].type != PT_TIN)
	{
		return false;
	}
	if (parts[i].temp >= 286.0f)
	{
		parts[i].tmp = 0;
		return false;
	}
	parts[i].tmp = std::min(parts[i].tmp + 1, 120);
	if (parts[i].tmp < 120)
	{
		return false;
	}
	if (!ConsumeEvent(sim))
	{
		parts[i].tmp = 119;
		return false;
	}

	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_MSCR);
	ResetReactionProduct(parts[i], PT_MSCR);
	parts[i].ctype = PT_TIN;
	parts[i].temp = temperature;
	return true;
}

bool ReactCarbonGroupWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_SLCN || sourceType == PT_FL)
	{
		return false;
	}
	auto properties = CarbonGroupPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotCarbonGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_FL)
	{
		return false;
	}
	auto properties = CarbonGroupPropertiesFor(sourceType);
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
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, properties.oxideProduct);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], properties.oxideProduct);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotCarbonGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_FL)
	{
		return false;
	}
	auto properties = CarbonGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool ExciteGermanium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_GE || parts[i].life > 0 ||
		!HasLocalDischarge(x, y, pmap, sim) ||
		!sim->rng.chance(1, 4) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 10;
	parts[i].temp = std::min(parts[i].temp + 15.0f, MAX_TEMP);
	return true;
}

bool UpdateCarbonGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsReactiveCarbonGroupElement(sourceType))
	{
		return false;
	}
	if (DecayFlerovium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (EmbrittleColdTin(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactCarbonGroupWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteGermanium(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayMoscovium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_MC || parts[i].type != PT_MC)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(50, 110);
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

	auto temperature = std::min(parts[i].temp + 600.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_NH);
	ResetReactionProduct(parts[i], PT_NH);
	parts[i].tmp = sim->rng.between(90, 180);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactNitrogenGroupWithAcid(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_AS && sourceType != PT_SB && sourceType != PT_BI)
	{
		return false;
	}
	auto properties = NitrogenGroupPropertiesFor(sourceType);
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
			{
				continue;
			}
			auto packed = pmap[y + ry][x + rx];
			if (!packed || TYP(packed) != PT_ACID)
			{
				continue;
			}
			auto acid = ID(packed);
			if (std::max(parts[i].temp, parts[acid].temp) < properties.acidThreshold)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}

			auto temperature = std::min(
				std::max(parts[i].temp, parts[acid].temp) +
					float(properties.reactionHeat),
				MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(acid, x + rx, y + ry, PT_H2);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[acid], PT_H2);
			parts[i].temp = temperature;
			parts[acid].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool OxidiseHotNitrogenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_N || sourceType == PT_MC)
	{
		return false;
	}
	auto properties = NitrogenGroupPropertiesFor(sourceType);
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
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, properties.oxideProduct);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], properties.oxideProduct);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			parts[oxygen].life = sourceType == PT_P ? 32 : 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotNitrogenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_N || sourceType == PT_MC)
	{
		return false;
	}
	auto properties = NitrogenGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool ExciteNitrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_N || parts[i].type != PT_N || parts[i].life > 0 ||
		!HasLocalDischarge(x, y, pmap, sim) ||
		!sim->rng.chance(1, 5) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 12;
	parts[i].temp = std::min(parts[i].temp + 20.0f, MAX_TEMP);
	return true;
}

bool UpdateNitrogenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsNitrogenGroupElement(sourceType))
	{
		return false;
	}
	if (DecayMoscovium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactNitrogenGroupWithAcid(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteNitrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayLivermorium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_LV || parts[i].type != PT_LV)
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sim->rng.between(40, 90);
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

	auto temperature = std::min(parts[i].temp + 650.0f, MAX_TEMP);
	sim->part_change_type(i, x, y, PT_FL);
	ResetReactionProduct(parts[i], PT_FL);
	parts[i].tmp = sim->rng.between(60, 130);
	parts[i].temp = temperature;
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = 0x03F03F00;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool OxidiseHotOxygenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_LV)
	{
		return false;
	}
	auto properties = OxygenGroupPropertiesFor(sourceType);
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
			auto temperature = std::min(
				parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, properties.oxideProduct);
			sim->part_change_type(oxygen, x + rx, y + ry, PT_FIRE);
			ResetReactionProduct(parts[i], properties.oxideProduct);
			ResetReactionProduct(parts[oxygen], PT_FIRE);
			parts[i].temp = temperature;
			parts[oxygen].temp = temperature;
			if (properties.oxideProduct == PT_SMKE)
			{
				parts[i].life = 45;
			}
			parts[oxygen].life = sourceType == PT_S ? 32 : 20;
			parts[oxygen].dcolour = properties.flameColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.35f);
			return true;
		}
	}
	return false;
}

bool VaporiseHotOxygenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_LV)
	{
		return false;
	}
	auto properties = OxygenGroupPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_FIRE);
	ResetReactionProduct(parts[i], PT_FIRE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.flameColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool ExciteSelenium(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_SE || parts[i].type != PT_SE || parts[i].life > 0 ||
		(!HasLocalDischarge(x, y, pmap, sim) && !HasLocalPhoton(x, y, sim)) ||
		!sim->rng.chance(1, 4) || !ConsumeEvent(sim))
	{
		return false;
	}
	parts[i].life = 12;
	parts[i].temp = std::min(parts[i].temp + 15.0f, MAX_TEMP);
	return true;
}

bool UpdateOxygenGroup(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsOxygenGroupExtension(sourceType))
	{
		return false;
	}
	if (DecayLivermorium(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (VaporiseHotOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (OxidiseHotOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return ExciteSelenium(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
}

bool DecayRadioactiveHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if ((sourceType != PT_AT && sourceType != PT_TS) ||
		(parts[i].type != sourceType &&
			(parts[i].type != PT_LAVA || parts[i].ctype != sourceType)))
	{
		return false;
	}
	if (parts[i].tmp <= 0)
	{
		parts[i].tmp = sourceType == PT_TS ? sim->rng.between(35, 75) :
			sim->rng.between(240, 480);
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

	auto properties = HalogenPropertiesFor(sourceType);
	auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
	int product = sourceType == PT_TS ? PT_MC : PT_POLO;
	sim->part_change_type(i, x, y, product);
	ResetReactionProduct(parts[i], product);
	parts[i].temp = temperature;
	if (product == PT_MC)
	{
		parts[i].tmp = sim->rng.between(50, 110);
	}
	int photon = sim->create_part(-3, x, y, PT_PHOT);
	if (photon >= 0)
	{
		parts[photon].ctype = sourceType == PT_TS ? 0x03F03F00 : 0x0003FFF0;
		parts[photon].temp = temperature;
		parts[photon].life = 18;
	}
	return true;
}

bool ReactFluorineWithWater(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_F || parts[i].type != PT_F || parts[i].temp < 250.0f)
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
			if (!packed || !IsWaterLike(TYP(packed)))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto water = ID(packed);
			auto properties = HalogenPropertiesFor(sourceType);
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_ACID);
			sim->part_change_type(water, x + rx, y + ry, PT_ACID);
			ResetReactionProduct(parts[i], PT_ACID);
			ResetReactionProduct(parts[water], PT_ACID);
			parts[i].temp = temperature;
			parts[water].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure);
			return true;
		}
	}
	return false;
}

bool ReactHalogenWithHydrogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_CHLR || sourceType == PT_TS)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.hydrogenThreshold)
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
			if (!packed || TYP(packed) != PT_H2)
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto hydrogen = ID(packed);
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_ACID);
			sim->part_change_type(hydrogen, x + rx, y + ry, PT_ACID);
			ResetReactionProduct(parts[i], PT_ACID);
			ResetReactionProduct(parts[hydrogen], PT_ACID);
			parts[i].temp = temperature;
			parts[hydrogen].temp = temperature;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.6f);
			return true;
		}
	}
	return false;
}

bool ReactHalogenWithMetal(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_TS)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.metalThreshold)
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
			if (!packed)
			{
				continue;
			}
			auto metal = ID(packed);
			auto metalType = parts[metal].type == PT_LAVA ? parts[metal].ctype : parts[metal].type;
			if (!IsHalogenReactiveMetal(metalType))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat), MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(metal, x + rx, y + ry, PT_SALT);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[metal], PT_SALT);
			parts[i].temp = temperature;
			parts[metal].temp = temperature;
			parts[i].dcolour = properties.vapourColour;
			parts[metal].dcolour = properties.vapourColour;
			AddBoundedPressure(sim, x, y, properties.pressure * 0.45f);
			return true;
		}
	}
	return false;
}

bool DisinfectWithHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType == PT_TS)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.disinfectionThreshold)
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
			if (!packed || !IsDisinfectionTarget(TYP(packed)))
			{
				continue;
			}
			if (!ConsumeEvent(sim))
			{
				return false;
			}
			auto target = ID(packed);
			auto temperature = std::min(parts[i].temp + float(properties.reactionHeat) * 0.25f, MAX_TEMP);
			sim->part_change_type(i, x, y, PT_SALT);
			sim->part_change_type(target, x + rx, y + ry, PT_DUST);
			ResetReactionProduct(parts[i], PT_SALT);
			ResetReactionProduct(parts[target], PT_DUST);
			parts[i].temp = temperature;
			parts[target].temp = temperature;
			return true;
		}
	}
	return false;
}

bool VaporiseHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (sourceType != PT_BR && sourceType != PT_I)
	{
		return false;
	}
	auto properties = HalogenPropertiesFor(sourceType);
	if (parts[i].temp < properties.boilingPoint || !ConsumeEvent(sim))
	{
		return false;
	}
	auto temperature = parts[i].temp;
	sim->part_change_type(i, x, y, PT_SMKE);
	ResetReactionProduct(parts[i], PT_SMKE);
	parts[i].ctype = sourceType;
	parts[i].temp = temperature;
	parts[i].life = 60;
	parts[i].dcolour = properties.vapourColour;
	AddBoundedPressure(sim, x, y, properties.pressure * 0.25f);
	return true;
}

bool UpdateHalogen(UPDATE_FUNC_ARGS, int sourceType)
{
	if (!IsHalogenElement(sourceType))
	{
		return false;
	}
	if (DecayRadioactiveHalogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactFluorineWithWater(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactHalogenWithHydrogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (ReactHalogenWithMetal(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	if (DisinfectWithHalogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType))
	{
		return true;
	}
	return VaporiseHalogen(UPDATE_FUNC_SUBCALL_ARGS, sourceType);
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

int OmniAlkalineEarthMetalUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenAlkalineEarthUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateAlkalineEarth(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

void OmniAlkalineEarthMetalCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_RA)
	{
		sim->parts[i].tmp = sim->rng.between(900, 1800);
	}
}

int OmniBoronGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenBoronGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateBoronGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

void OmniBoronGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_NH)
	{
		sim->parts[i].tmp = sim->rng.between(90, 180);
	}
}

int OmniCarbonGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenCarbonGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateCarbonGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniCarbonGroupGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_GE && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 12, 120);
		*firea = intensity;
		*firer = std::min(*colr + 40, 255);
		*fireg = std::min(*colg + 55, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_FL)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniCarbonGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_FL)
	{
		sim->parts[i].tmp = sim->rng.between(60, 130);
	}
}

int OmniNitrogenGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenNitrogenGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateNitrogenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniNitrogenGroupGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_N && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 10, 120);
		*firea = intensity;
		*firer = std::min(*colr + 60, 255);
		*fireg = std::min(*colg + 20, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_MC)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniNitrogenGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_MC)
	{
		sim->parts[i].tmp = sim->rng.between(50, 110);
	}
}

int OmniOxygenGroupUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenOxygenGroupUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateOxygenGroup(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniOxygenGroupGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_SE && cpart->life > 0)
	{
		int intensity = std::min(cpart->life * 10, 120);
		*firea = intensity;
		*firer = std::min(*colr + 40, 255);
		*fireg = std::min(*colg + 70, 255);
		*fireb = 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if (cpart->type == PT_LV)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniOxygenGroupCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_LV)
	{
		sim->parts[i].tmp = sim->rng.between(40, 90);
	}
}

int OmniHalogenUpdate(UPDATE_FUNC_ARGS)
{
	return UpdateHalogen(UPDATE_FUNC_SUBCALL_ARGS, parts[i].type) ? 1 : 0;
}

int OmniMoltenHalogenUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LAVA)
	{
		return 0;
	}
	return UpdateHalogen(UPDATE_FUNC_SUBCALL_ARGS, parts[i].ctype) ? 1 : 0;
}

int OmniHalogenGraphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->type == PT_AT || cpart->type == PT_TS)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void OmniHalogenCreate(ELEMENT_CREATE_FUNC_ARGS)
{
	if (t == PT_AT)
	{
		sim->parts[i].tmp = sim->rng.between(240, 480);
	}
	else if (t == PT_TS)
	{
		sim->parts[i].tmp = sim->rng.between(35, 75);
	}
}
