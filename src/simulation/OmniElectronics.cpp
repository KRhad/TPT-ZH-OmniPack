#include "OmniElectronics.h"

#include "ElementCommon.h"
#include "OmniModuleRuntime.h"
#include "SimulationData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

namespace
{
constexpr int ElectronicsEventsPerFrame = 1024;
thread_local OmniModuleRuntimeCache electronicsRuntimeCache;

struct EventBudget
{
	Simulation *simulation = nullptr;
	int tick = -1;
	int remaining = ElectronicsEventsPerFrame;
};

thread_local EventBudget eventBudget;

struct Slot
{
	int index = -1;
	int x = -1;
	int y = -1;
};

bool ConsumeEvent(Simulation *sim)
{
	if (eventBudget.simulation != sim || eventBudget.tick != sim->currentTick)
	{
		eventBudget.simulation = sim;
		eventBudget.tick = sim->currentTick;
		eventBudget.remaining = ElectronicsEventsPerFrame;
	}
	if (eventBudget.remaining <= 0)
		return false;
	--eventBudget.remaining;
	sim->RecordOmniEvent();
	return true;
}

bool IsTouched(int index, Parts &parts, Simulation *sim)
{
	return index >= 0 && parts[index].tmp3 == sim->currentTick + 1;
}

void Touch(int index, Parts &parts, Simulation *sim)
{
	if (index >= 0)
		parts[index].tmp3 = sim->currentTick + 1;
}

bool IsExcluded(int index, std::array<int, 5> const &excluded, int count)
{
	for (int position = 0; position < count; ++position)
	{
		if (excluded[position] == index)
			return true;
	}
	return false;
}

Slot FindLocal(
	int x,
	int y,
	int type,
	std::array<int, 5> const &excluded,
	int excludedCount,
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
			if (IsTouched(index, parts, sim)
				|| IsExcluded(index, excluded, excludedCount))
				continue;
			return { index, x + rx, y + ry };
		}
	}
	return {};
}

Slot FindLocal(int x, int y, int type, int excluded, Parts &parts,
	int pmap[YRES][XRES], Simulation *sim)
{
	return FindLocal(x, y, type, { excluded, -1, -1, -1, -1 }, 1, parts, pmap, sim);
}

Slot FindLocalReagent(
	int x,
	int y,
	int type,
	std::array<int, 5> const &excluded,
	int excludedCount,
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
			auto actualType = TYP(packed);
			if (actualType != type
				&& !(actualType == PT_LAVA && parts[index].ctype == type))
				continue;
			if (IsTouched(index, parts, sim)
				|| IsExcluded(index, excluded, excludedCount))
				continue;
			return { index, x + rx, y + ry };
		}
	}
	return {};
}

Slot FindLocalConductor(
	int x,
	int y,
	std::array<int, 5> const &excluded,
	int excludedCount,
	Parts &parts,
	int pmap[YRES][XRES],
	Simulation *sim)
{
	auto const &elements = SimulationData::CRef().elements;
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
			if (type == PT_SPRK || IsTouched(index, parts, sim)
				|| IsExcluded(index, excluded, excludedCount)
				|| !(elements[type].Properties & PROP_CONDUCTS))
				continue;
			return { index, x + rx, y + ry };
		}
	}
	return {};
}

Slot FindLocalPhoton(int x, int y, Parts &parts, Simulation *sim)
{
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if (!InBounds(x + rx, y + ry))
				continue;
			auto packed = sim->photons[y + ry][x + rx];
			if (packed && TYP(packed) == PT_PHOT
				&& !IsTouched(ID(packed), parts, sim))
				return { ID(packed), x + rx, y + ry };
		}
	}
	return {};
}

void InitialiseState(Particle &part, int type)
{
	part.life = 0;
	part.ctype = PT_NONE;
	part.tmp = 0;
	part.tmp2 = 0;
	part.tmp3 = 0;
	part.tmp4 = 0;
	if (type == PT_LCOB)
		part.tmp = 100;
	else if (type == PT_PCMT)
		part.tmp = 1;
}

bool Convert(Simulation *sim, Slot slot, int type, Parts &parts, float temperature)
{
	if (slot.index < 0 || sim->part_change_type(slot.index, slot.x, slot.y, type))
		return false;
	InitialiseState(parts[slot.index], type);
	parts[slot.index].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	return true;
}

bool CatalyticSynthesis(
	UPDATE_FUNC_ARGS,
	std::initializer_list<int> inputTypes,
	int productType,
	float minimumTemperature,
	float maximumTemperature,
	float minimumPressure = -MAX_PRESSURE)
{
	if ((parts[i].type != PT_CATA
			&& !(parts[i].type == PT_SPRK && parts[i].ctype == PT_CATA))
		|| parts[i].temp < minimumTemperature
		|| parts[i].temp > maximumTemperature
		|| sim->pv[y / CELL][x / CELL] < minimumPressure
		|| inputTypes.size() > 4)
		return false;

	std::array<Slot, 4> inputs{};
	std::array<int, 5> excluded{ i, -1, -1, -1, -1 };
	int count = 0;
	for (auto type : inputTypes)
	{
		auto slot = FindLocalReagent(
			x, y, type, excluded, count + 1, parts, pmap, sim);
		if (slot.index < 0)
			return false;
		inputs[count] = slot;
		excluded[count + 1] = slot.index;
		++count;
	}
	if (!ConsumeEvent(sim))
		return false;

	float temperature = parts[i].temp;
	for (int position = 0; position < count; ++position)
		temperature += parts[inputs[position].index].temp;
	temperature /= static_cast<float>(count + 1);
	for (int position = 0; position < count; ++position)
	{
		Convert(sim, inputs[position], productType, parts, temperature);
		Touch(inputs[position].index, parts, sim);
	}
	return true;
}

bool RunCatalystRecipes(UPDATE_FUNC_ARGS)
{
	// More-specific recipes precede overlapping two-input routes.
	return CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_Y, PT_BA, PT_CUOX, PT_O2 },
			PT_SUPC, 1150.0f, 1900.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_LITH, PT_LA, PT_ZR, PT_O2 },
			PT_SELE, 1300.0f, 2200.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_ND, PT_IRON, PT_B },
			PT_PMAG, 1250.0f, 1900.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_LEAD, PT_ZR, PT_TIOX },
			PT_PZCR, 1150.0f, 1900.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_LITH, PT_COBT, PT_O2 },
			PT_LCOB, 900.0f, 1600.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_IN, PT_TIN, PT_O2 },
			PT_ITOX, 850.0f, 1500.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_GE, PT_SB, PT_TE },
			PT_PCMT, 850.0f, 1500.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_TUNG, PT_O2, PT_QRTZ },
			PT_ECHR, 950.0f, 1800.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_GA, PT_AS },
			PT_GAAS, 1100.0f, 1700.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_GA, PT_N },
			PT_GANI, 1300.0f, 2100.0f, 2.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_FEOX, PT_ZNOX },
			PT_FRIT, 900.0f, 1500.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_IRON, PT_SLCN },
			PT_SMAG, 1050.0f, 1700.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_BI, PT_TE },
			PT_TELC, 650.0f, 1200.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_GRPH, PT_GRPH },
			PT_CNTB, 900.0f, 1500.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_CNTB, PT_EPXY },
			PT_CFRP, 330.0f, 550.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_ERES, PT_PSTY },
			PT_PHRS, 330.0f, 500.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_BA, PT_TIOX },
			PT_DIEL, 1000.0f, 1700.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_QRTZ, PT_WATR },
			PT_AERG, 350.0f, 520.0f, -20.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_COAL },
			PT_GRAN, 1500.0f, 2400.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_GRAN },
			PT_GRPH, 900.0f, 1500.0f, 3.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_SLCN, PT_B },
			PT_PSCN, 850.0f, 1400.0f)
		|| CatalyticSynthesis(
			UPDATE_FUNC_SUBCALL_ARGS, { PT_SLCN, PT_P },
			PT_NSCN, 850.0f, 1400.0f);
}

bool SparkSelf(Simulation *sim, int i, int x, int y, int sourceType, Parts &parts)
{
	auto savedTmp = parts[i].tmp;
	auto savedTmp2 = parts[i].tmp2;
	auto savedTmp4 = parts[i].tmp4;
	if (sim->part_change_type(i, x, y, PT_SPRK))
		return false;
	parts[i].life = 4;
	parts[i].ctype = sourceType;
	parts[i].tmp = savedTmp;
	parts[i].tmp2 = savedTmp2;
	parts[i].tmp4 = savedTmp4;
	return true;
}

bool Photoconduction(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_GAAS || IsTouched(i, parts, sim))
		return false;
	auto photon = FindLocalPhoton(x, y, parts, sim);
	if (photon.index < 0 || !ConsumeEvent(sim))
		return false;
	Touch(photon.index, parts, sim);
	return SparkSelf(sim, i, x, y, PT_GAAS, parts);
}

bool ThermoelectricGradient(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_TELC || IsTouched(i, parts, sim))
		return false;
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			auto packed = pmap[y + ry][x + rx];
			if (!packed)
				continue;
			auto other = ID(packed);
			auto difference = parts[other].temp - parts[i].temp;
			if (std::fabs(difference) < 120.0f || !ConsumeEvent(sim))
				continue;
			auto transfer = std::clamp(difference * 0.05f, -8.0f, 8.0f);
			parts[i].temp += transfer;
			parts[other].temp -= transfer;
			Touch(other, parts, sim);
			return SparkSelf(sim, i, x, y, PT_TELC, parts);
		}
	}
	return false;
}

bool ColdSuperconduct(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SUPC || parts[i].temp >= 120.0f
		|| IsTouched(i, parts, sim))
		return false;
	auto spark = FindLocal(x, y, PT_SPRK, i, parts, pmap, sim);
	if (spark.index < 0 || parts[spark.index].life <= 0
		|| !ConsumeEvent(sim))
		return false;
	Touch(spark.index, parts, sim);
	return SparkSelf(sim, i, x, y, PT_SUPC, parts);
}

bool PiezoelectricPressure(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PZCR || IsTouched(i, parts, sim)
		|| std::fabs(sim->pv[y / CELL][x / CELL]) < 5.0f
		|| !ConsumeEvent(sim))
		return false;
	return SparkSelf(sim, i, x, y, PT_PZCR, parts);
}

bool MagneticDeflection(UPDATE_FUNC_ARGS)
{
	auto type = parts[i].type;
	if (type != PT_PMAG && type != PT_SMAG)
		return false;
	if (type == PT_SMAG && parts[i].life <= 0)
	{
		auto spark = FindLocal(x, y, PT_SPRK, i, parts, pmap, sim);
		if (spark.index >= 0 && ConsumeEvent(sim))
			parts[i].life = 90;
		else
			return false;
	}
	for (int ry = -1; ry <= 1; ++ry)
	{
		for (int rx = -1; rx <= 1; ++rx)
		{
			if ((!rx && !ry) || !InBounds(x + rx, y + ry))
				continue;
			for (auto packed : {
				pmap[y + ry][x + rx], sim->photons[y + ry][x + rx] })
			{
				if (!packed)
					continue;
				auto neighbour = ID(packed);
				auto neighbourType = TYP(packed);
				if (IsTouched(neighbour, parts, sim))
					continue;
				if (neighbourType == PT_ELEC && ConsumeEvent(sim))
				{
					auto oldVx = parts[neighbour].vx;
					parts[neighbour].vx = -parts[neighbour].vy;
					parts[neighbour].vy = oldVx;
					Touch(neighbour, parts, sim);
					return true;
				}
				if ((neighbourType == PT_IRON || neighbourType == PT_BMTL)
					&& ConsumeEvent(sim))
				{
					parts[neighbour].vx -= static_cast<float>(rx) * 0.08f;
					parts[neighbour].vy -= static_cast<float>(ry) * 0.08f;
					Touch(neighbour, parts, sim);
					return true;
				}
			}
		}
	}
	return type == PT_SMAG && parts[i].life > 0;
}

bool CarbonNanomaterialBehaviour(UPDATE_FUNC_ARGS)
{
	if (parts[i].type == PT_GRPH && parts[i].temp >= 900.0f)
	{
		auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
		if (oxygen.index >= 0 && ConsumeEvent(sim))
		{
			sim->kill_part(oxygen.index);
			Convert(sim, { i, x, y }, PT_CO2, parts, parts[i].temp);
			return true;
		}
	}
	if (parts[i].type == PT_CNTB
		&& std::fabs(sim->pv[y / CELL][x / CELL]) >= 24.0f
		&& ConsumeEvent(sim))
	{
		Convert(sim, { i, x, y }, PT_GRPH, parts, parts[i].temp);
		return true;
	}
	if (parts[i].type == PT_AERG
		&& std::fabs(sim->pv[y / CELL][x / CELL]) >= 10.0f
		&& ConsumeEvent(sim))
	{
		Convert(sim, { i, x, y }, PT_QRTZ, parts, parts[i].temp);
		return true;
	}
	return false;
}

bool BatteryThermalRunaway(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_LCOB || parts[i].temp < 650.0f)
		return false;
	auto oxygen = FindLocal(x, y, PT_O2, i, parts, pmap, sim);
	if (oxygen.index < 0 || !ConsumeEvent(sim))
		return false;
	Convert(sim, oxygen, PT_FIRE, parts, std::max(parts[i].temp, 900.0f));
	Convert(sim, { i, x, y }, PT_COBT, parts, std::max(parts[i].temp, 800.0f));
	sim->pv[y / CELL][x / CELL] = std::min(
		sim->pv[y / CELL][x / CELL] + 0.5f, MAX_PRESSURE);
	return true;
}

bool PhaseChangeMemory(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PCMT || IsTouched(i, parts, sim))
		return false;
	if (parts[i].temp >= 850.0f && parts[i].tmp != 0 && ConsumeEvent(sim))
	{
		parts[i].tmp = 0;
		parts[i].life = 30;
		return true;
	}
	if (parts[i].temp >= 500.0f && parts[i].temp <= 700.0f
		&& parts[i].tmp == 0 && parts[i].life <= 0 && ConsumeEvent(sim))
	{
		parts[i].tmp = 1;
		return true;
	}
	if (parts[i].tmp == 1)
	{
		auto spark = FindLocal(x, y, PT_SPRK, i, parts, pmap, sim);
		if (spark.index >= 0 && ConsumeEvent(sim))
			return SparkSelf(sim, i, x, y, PT_PCMT, parts);
	}
	return false;
}

bool ElectrochromicToggle(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_ECHR || parts[i].life > 0 || IsTouched(i, parts, sim))
		return false;
	auto spark = FindLocal(x, y, PT_SPRK, i, parts, pmap, sim);
	if (spark.index < 0 || !ConsumeEvent(sim))
		return false;
	parts[i].tmp = parts[i].tmp ? 0 : 1;
	parts[i].life = 20;
	Touch(spark.index, parts, sim);
	return true;
}

bool PhotoresistExposure(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_PHRS || IsTouched(i, parts, sim))
		return false;
	auto photon = FindLocalPhoton(x, y, parts, sim);
	if (photon.index >= 0 && parts[i].tmp < 100 && ConsumeEvent(sim))
	{
		parts[i].tmp = std::min(parts[i].tmp + 25, 100);
		Touch(photon.index, parts, sim);
		return true;
	}
	auto solvent = FindLocal(x, y, PT_ACET, i, parts, pmap, sim);
	if (solvent.index >= 0 && parts[i].tmp >= 50 && ConsumeEvent(sim))
	{
		sim->kill_part(i);
		return true;
	}
	return false;
}

bool DielectricCharge(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_DIEL || IsTouched(i, parts, sim))
		return false;
	if (parts[i].tmp >= 8)
	{
		auto output = FindLocalConductor(
			x, y, { i, -1, -1, -1, -1 }, 1, parts, pmap, sim);
		if (output.index >= 0 && ConsumeEvent(sim))
		{
			auto sourceType = parts[output.index].type;
			if (!sim->part_change_type(output.index, output.x, output.y, PT_SPRK))
			{
				parts[output.index].ctype = sourceType;
				parts[output.index].life = 4;
				parts[i].tmp = 0;
				Touch(output.index, parts, sim);
				return true;
			}
		}
	}
	if (parts[i].life > 0)
		return false;
	auto input = FindLocal(x, y, PT_SPRK, i, parts, pmap, sim);
	if (input.index < 0 || !ConsumeEvent(sim))
		return false;
	parts[i].tmp = std::min(parts[i].tmp + 1, 8);
	parts[i].life = 4;
	Touch(input.index, parts, sim);
	return true;
}

bool TransferBatteryCharge(UPDATE_FUNC_ARGS, int sourceType, int targetType)
{
	if (parts[i].ctype != sourceType || parts[i].life != 3)
		return false;
	auto electrolyte = FindLocal(x, y, PT_SELE, i, parts, pmap, sim);
	auto target = FindLocal(
		x, y, targetType,
		{ i, electrolyte.index, -1, -1, -1 }, 2, parts, pmap, sim);
	if (electrolyte.index < 0 || target.index < 0)
		return false;
	if (parts[i].tmp <= 0 || parts[target.index].tmp >= 100
		|| !ConsumeEvent(sim))
		return false;
	parts[i].tmp = std::max(parts[i].tmp - 5, 0);
	parts[target.index].tmp = std::min(parts[target.index].tmp + 5, 100);
	parts[i].temp = std::min(parts[i].temp + 2.0f, MAX_TEMP);
	parts[target.index].temp = std::min(parts[target.index].temp + 2.0f, MAX_TEMP);
	Touch(electrolyte.index, parts, sim);
	Touch(target.index, parts, sim);
	return true;
}

void ConfigureSolid(
	Element &element,
	RGB colour,
	int weight,
	int heatConduct,
	float heatCapacity,
	int hardness,
	float highTemperature,
	int highTransition,
	unsigned int properties)
{
	element.Colour = colour;
	element.Advection = 0.0f;
	element.AirDrag = 0.0f;
	element.AirLoss = 0.90f;
	element.Loss = 0.0f;
	element.Collision = 0.0f;
	element.Gravity = 0.0f;
	element.Diffusion = 0.0f;
	element.HotAir = 0.0f;
	element.Falldown = 0;
	element.Flammable = 0;
	element.Explosive = 0;
	element.Meltable = highTransition == PT_LAVA;
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
	element.HighTemperature = highTemperature;
	element.HighTemperatureTransition = highTransition;
	element.Update = &OmniElectronicsElementUpdate;
	element.Graphics = &OmniElectronicsGraphics;
}
}

bool OmniElectronicsModuleEnabled(Simulation *sim)
{
	return OmniModuleRuntimeEnabled(
		electronicsRuntimeCache, sim, sim->currentTick, "Omni.Modules.Electronics");
}

bool OmniIsElectronicsElement(int type)
{
	return type >= PT_GAAS && type <= PT_DIEL;
}

void OmniConfigureElectronicsElement(Element &element, int type)
{
	switch (type)
	{
	case PT_GAAS:
		ConfigureSolid(element, 0x8E6F9E_rgb, 105, 46, 0.35f, 45, 1511.0f, PT_LAVA,
			TYPE_SOLID | PROP_NEUTPASS);
		break;
	case PT_GANI:
		ConfigureSolid(element, 0x6F88B8_rgb, 92, 58, 0.42f, 62, 2773.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	case PT_FRIT:
		ConfigureSolid(element, 0x5B5148_rgb, 108, 18, 0.72f, 70, 1650.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTABSORB | PROP_LIFE_DEC);
		break;
	case PT_PMAG:
		ConfigureSolid(element, 0x6E7786_rgb, 135, 72, 0.48f, 80, 1350.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW | PROP_LIFE_DEC);
		break;
	case PT_SMAG:
		ConfigureSolid(element, 0x76828B_rgb, 118, 88, 0.50f, 72, 1700.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW | PROP_LIFE_DEC);
		break;
	case PT_PZCR:
		ConfigureSolid(element, 0xD2C9B0_rgb, 128, 24, 0.76f, 78, 1550.0f, PT_LAVA,
			TYPE_SOLID | PROP_NEUTPASS);
		break;
	case PT_TELC:
		ConfigureSolid(element, 0x7B748F_rgb, 122, 32, 0.44f, 58, 900.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC);
		break;
	case PT_SUPC:
		ConfigureSolid(element, 0x5DA9C5_rgb, 115, 248, 0.62f, 64, 1250.0f, PT_LAVA,
			TYPE_SOLID | PROP_NEUTPASS);
		break;
	case PT_GRPH:
		ConfigureSolid(element, 0x24272A_rgb, 48, 250, 0.38f, 36, 3900.0f, PT_CO2,
			TYPE_SOLID | PROP_CONDUCTS | PROP_PHOTPASS | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	case PT_CNTB:
		ConfigureSolid(element, 0x30363A_rgb, 42, 220, 0.32f, 92, 4100.0f, PT_CO2,
			TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	case PT_AERG:
		ConfigureSolid(element, 0xD8EDF0_rgb, 8, 1, 0.12f, 8, 1300.0f, PT_QRTZ,
			TYPE_SOLID | PROP_PHOTPASS | PROP_NEUTPASS);
		break;
	case PT_CFRP:
		ConfigureSolid(element, 0x2C3034_rgb, 76, 52, 0.68f, 95, 780.0f, PT_SMKE,
			TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	case PT_LCOB:
		ConfigureSolid(element, 0x46526E_rgb, 112, 36, 0.84f, 60, 1050.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC);
		element.DefaultProperties.tmp = 100;
		break;
	case PT_GRAN:
		ConfigureSolid(element, 0x343536_rgb, 66, 180, 0.52f, 42, 3600.0f, PT_CO2,
			TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	case PT_SELE:
		ConfigureSolid(element, 0xD7D1BE_rgb, 104, 12, 0.90f, 76, 1850.0f, PT_LAVA,
			TYPE_SOLID | PROP_NEUTPASS);
		break;
	case PT_ITOX:
		ConfigureSolid(element, 0x9FD2D6_rgb, 118, 80, 0.56f, 54, 1800.0f, PT_LAVA,
			TYPE_SOLID | PROP_CONDUCTS | PROP_PHOTPASS | PROP_LIFE_DEC);
		break;
	case PT_PCMT:
		ConfigureSolid(element, 0x654F70_rgb, 126, 28, 0.54f, 50, 1100.0f, PT_LAVA,
			TYPE_SOLID | PROP_NEUTPASS | PROP_LIFE_DEC);
		element.DefaultProperties.tmp = 1;
		break;
	case PT_ECHR:
		ConfigureSolid(element, 0x9CB7C2_rgb, 110, 16, 0.72f, 48, 1200.0f, PT_LAVA,
			TYPE_SOLID | PROP_PHOTPASS | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	case PT_PHRS:
		ConfigureSolid(element, 0xB98FAE_rgb, 82, 6, 0.70f, 24, 520.0f, PT_SMKE,
			TYPE_SOLID | PROP_NEUTPASS);
		break;
	case PT_DIEL:
		ConfigureSolid(element, 0xE0D8C5_rgb, 120, 8, 0.92f, 82, 1900.0f, PT_LAVA,
			TYPE_SOLID | PROP_NEUTPASS | PROP_LIFE_DEC);
		break;
	default:
		break;
	}
}

int OmniElectronicsCatalystUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniElectronicsModuleEnabled(sim))
		return 0;
	return RunCatalystRecipes(UPDATE_FUNC_SUBCALL_ARGS) ? 1 : 0;
}

int OmniElectronicsElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniElectronicsModuleEnabled(sim) || IsTouched(i, parts, sim))
		return 0;
	if (Photoconduction(UPDATE_FUNC_SUBCALL_ARGS)
		|| ThermoelectricGradient(UPDATE_FUNC_SUBCALL_ARGS)
		|| ColdSuperconduct(UPDATE_FUNC_SUBCALL_ARGS)
		|| PiezoelectricPressure(UPDATE_FUNC_SUBCALL_ARGS)
		|| MagneticDeflection(UPDATE_FUNC_SUBCALL_ARGS)
		|| CarbonNanomaterialBehaviour(UPDATE_FUNC_SUBCALL_ARGS)
		|| BatteryThermalRunaway(UPDATE_FUNC_SUBCALL_ARGS)
		|| PhaseChangeMemory(UPDATE_FUNC_SUBCALL_ARGS)
		|| ElectrochromicToggle(UPDATE_FUNC_SUBCALL_ARGS)
		|| PhotoresistExposure(UPDATE_FUNC_SUBCALL_ARGS)
		|| DielectricCharge(UPDATE_FUNC_SUBCALL_ARGS))
		return 1;
	return 0;
}

int OmniElectronicsSparkUpdate(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_SPRK)
		return 0;
	if (parts[i].ctype == PT_CATA)
		return OmniElectronicsCatalystUpdate(UPDATE_FUNC_SUBCALL_ARGS);
	if (!OmniElectronicsModuleEnabled(sim)
		|| !OmniIsElectronicsElement(parts[i].ctype))
		return 0;

	if (parts[i].ctype == PT_FRIT && parts[i].life > 0)
	{
		if (parts[i].life == 3 && ConsumeEvent(sim))
			parts[i].temp = std::min(parts[i].temp + 8.0f, MAX_TEMP);
		return 1;
	}
	if (parts[i].ctype == PT_GANI && parts[i].life == 3 && ConsumeEvent(sim))
	{
		auto photon = sim->create_part(-3, x, y, PT_PHOT);
		if (photon >= 0)
		{
			parts[photon].ctype = 0x0003FFF0;
			parts[photon].life = 12;
			parts[photon].vx = 1.0f;
			parts[photon].vy = 0.0f;
		}
	}
	if (parts[i].ctype == PT_PZCR && parts[i].life == 3 && ConsumeEvent(sim))
	{
		auto direction = sim->pv[y / CELL][x / CELL] < 0.0f ? -1.0f : 1.0f;
		sim->pv[y / CELL][x / CELL] = std::clamp(
			sim->pv[y / CELL][x / CELL] + direction * 0.4f,
			MIN_PRESSURE, MAX_PRESSURE);
	}
	if (parts[i].ctype == PT_SUPC)
		parts[i].temp = std::min(parts[i].temp, 120.0f);
	if (parts[i].ctype == PT_LCOB)
		TransferBatteryCharge(
			UPDATE_FUNC_SUBCALL_ARGS, PT_LCOB, PT_GRAN);
	else if (parts[i].ctype == PT_GRAN)
		TransferBatteryCharge(
			UPDATE_FUNC_SUBCALL_ARGS, PT_GRAN, PT_LCOB);
	return 0;
}

int OmniElectronicsGraphics(GRAPHICS_FUNC_ARGS)
{
	switch (cpart->type)
	{
	case PT_SMAG:
		if (cpart->life > 0)
		{
			*colb = std::min(*colb + 45, 255);
			*pixel_mode |= PMODE_GLOW;
		}
		break;
	case PT_LCOB:
		*colg = std::clamp(*colg + cpart->tmp / 3, 0, 255);
		break;
	case PT_GRAN:
		*colb = std::clamp(*colb + cpart->tmp / 4, 0, 255);
		break;
	case PT_PCMT:
		if (!cpart->tmp)
		{
			*colr /= 2;
			*colg /= 2;
			*colb /= 2;
		}
		break;
	case PT_ECHR:
		if (cpart->tmp)
		{
			*colr = (*colr * 2) / 5;
			*colg = (*colg * 2) / 5;
			*colb = std::min((*colb * 3) / 5 + 30, 255);
		}
		break;
	case PT_PHRS:
		*colb = std::clamp(*colb + cpart->tmp / 2, 0, 255);
		break;
	case PT_DIEL:
		if (cpart->tmp > 0)
			*pixel_mode |= PMODE_GLOW;
		break;
	default:
		break;
	}
	return 0;
}
