#include "OmniOrganics.h"

#include "ElementCommon.h"
#include "OmniChemistry.h"

#include <algorithm>
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

Slot FindLocal(
	int x,
	int y,
	int type,
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
			if (!packed || TYP(packed) != type)
				continue;
			auto index = ID(packed);
			if (IsTouched(index, parts, sim)
				|| std::find(excluded.begin(), excluded.end(), index) != excluded.end())
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
			if ((rx || ry) && InBounds(x + rx, y + ry)
				&& !pmap[y + ry][x + rx])
				return { -1, x + rx, y + ry };
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
		return false;
	parts[slot.index].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
	parts[slot.index].life = 0;
	parts[slot.index].ctype = PT_NONE;
	parts[slot.index].tmp = 0;
	parts[slot.index].tmp2 = 0;
	parts[slot.index].tmp3 = 0;
	parts[slot.index].tmp4 = 0;
	return true;
}

int CreateProduct(
	Simulation *sim,
	Slot slot,
	int type,
	Parts &parts,
	float temperature)
{
	if (slot.x < 0)
		return -1;
	auto created = sim->create_part(-1, slot.x, slot.y, type);
	if (created >= 0)
	{
		parts[created].temp = restrict_flt(temperature, MIN_TEMP, MAX_TEMP);
		Touch(created, parts, sim);
	}
	return created;
}

float ReactionTemperature(Parts &parts, std::initializer_list<int> indices)
{
	float temperature = 0.0f;
	int count = 0;
	for (auto index : indices)
	{
		if (index < 0)
			continue;
		temperature += parts[index].temp;
		++count;
	}
	return count ? temperature / count : 293.15f;
}

bool MethaneSynthesis(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA || parts[i].temp < 500.0f
		|| parts[i].temp > 850.0f)
		return false;
	auto carbonDioxide = FindLocal(x, y, PT_CO2, { i }, parts, pmap, sim);
	auto hydrogen = FindLocal(
		x, y, PT_H2, { i, carbonDioxide.index }, parts, pmap, sim);
	if (carbonDioxide.index < 0 || hydrogen.index < 0
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(
		parts, { i, carbonDioxide.index, hydrogen.index });
	auto productTemperature = std::min(temperature, 350.0f);
	Convert(sim, carbonDioxide, PT_CH4M, parts, productTemperature);
	Convert(sim, hydrogen, PT_WATR, parts, productTemperature);
	Touch(carbonDioxide.index, parts, sim);
	Touch(hydrogen.index, parts, sim);
	return true;
}

bool AcetyleneCyclisation(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA || parts[i].temp < 750.0f)
		return false;
	auto first = FindLocal(x, y, PT_ACTY, { i }, parts, pmap, sim);
	auto second = FindLocal(x, y, PT_ACTY, { i, first.index }, parts, pmap, sim);
	auto third = FindLocal(
		x, y, PT_ACTY, { i, first.index, second.index }, parts, pmap, sim);
	if (first.index < 0 || second.index < 0 || third.index < 0
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(
		parts, { i, first.index, second.index, third.index });
	Convert(sim, first, PT_BENZ, parts, std::min(temperature, 340.0f));
	sim->kill_part(second.index);
	sim->kill_part(third.index);
	Touch(first.index, parts, sim);
	return true;
}

bool UreaSynthesis(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CATA || parts[i].temp < 430.0f
		|| parts[i].temp > 650.0f || sim->pv[y / CELL][x / CELL] < 2.0f)
		return false;
	auto first = FindLocal(x, y, PT_AMON, { i }, parts, pmap, sim);
	auto second = FindLocal(x, y, PT_AMON, { i, first.index }, parts, pmap, sim);
	auto carbonDioxide = FindLocal(
		x, y, PT_CO2, { i, first.index, second.index }, parts, pmap, sim);
	if (first.index < 0 || second.index < 0 || carbonDioxide.index < 0
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(
		parts, { i, first.index, second.index, carbonDioxide.index });
	auto productTemperature = std::min(temperature, 390.0f);
	Convert(sim, first, PT_UREA, parts, productTemperature);
	Convert(sim, second, PT_WATR, parts, productTemperature);
	sim->kill_part(carbonDioxide.index);
	Touch(first.index, parts, sim);
	Touch(second.index, parts, sim);
	return true;
}

bool MethaneSteamReforming(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_CH4M || parts[i].temp < 850.0f)
		return false;
	auto steam = FindLocal(x, y, PT_WTRV, { i }, parts, pmap, sim);
	auto catalyst = FindLocal(x, y, PT_CATA, { i, steam.index }, parts, pmap, sim);
	if (steam.index < 0 || catalyst.index < 0 || parts[catalyst.index].temp < 850.0f
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(parts, { i, steam.index, catalyst.index });
	Convert(sim, { i, x, y }, PT_H2, parts, temperature);
	Convert(sim, steam, PT_COMO, parts, temperature);
	Touch(i, parts, sim);
	Touch(steam.index, parts, sim);
	return true;
}

bool CrackHydrocarbon(
	UPDATE_FUNC_ARGS,
	int sourceType,
	int secondProduct,
	float minimumTemperature)
{
	if (parts[i].type != sourceType || parts[i].temp < minimumTemperature)
		return false;
	auto catalyst = FindLocal(x, y, PT_CATA, { i }, parts, pmap, sim);
	auto empty = FindEmpty(x, y, pmap);
	if (catalyst.index < 0 || parts[catalyst.index].temp < minimumTemperature
		|| empty.x < 0 || !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(parts, { i, catalyst.index });
	if (CreateProduct(sim, empty, secondProduct, parts, temperature) < 0)
		return false;
	Convert(sim, { i, x, y }, PT_ETHE, parts, temperature);
	Touch(i, parts, sim);
	return true;
}

bool EthylenePolymerisation(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_ETHE || parts[i].temp < 430.0f
		|| parts[i].temp > 650.0f || IsTouched(i, parts, sim))
		return false;
	auto second = FindLocal(x, y, PT_ETHE, { i }, parts, pmap, sim);
	auto catalyst = FindLocal(x, y, PT_CATA, { i, second.index }, parts, pmap, sim);
	if (second.index < 0 || catalyst.index < 0
		|| parts[catalyst.index].temp < 430.0f
		|| parts[catalyst.index].temp > 650.0f
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(parts, { i, second.index, catalyst.index });
	Convert(sim, { i, x, y }, PT_POLY, parts, temperature);
	Convert(sim, second, PT_POLY, parts, temperature);
	Touch(i, parts, sim);
	Touch(second.index, parts, sim);
	return true;
}

bool AlcoholOxidation(
	UPDATE_FUNC_ARGS,
	int sourceType,
	int productType,
	float minimumTemperature,
	float maximumTemperature)
{
	if (parts[i].type != sourceType || parts[i].temp < minimumTemperature
		|| parts[i].temp > maximumTemperature || IsTouched(i, parts, sim))
		return false;
	auto oxygen = FindLocal(x, y, PT_O2, { i }, parts, pmap, sim);
	auto catalyst = FindLocal(x, y, PT_CATA, { i, oxygen.index }, parts, pmap, sim);
	if (oxygen.index < 0 || catalyst.index < 0
		|| parts[catalyst.index].temp < minimumTemperature
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = std::min(
		ReactionTemperature(parts, { i, oxygen.index, catalyst.index }), 350.0f);
	Convert(sim, { i, x, y }, productType, parts, temperature);
	Convert(sim, oxygen, PT_WATR, parts, temperature);
	Touch(i, parts, sim);
	Touch(oxygen.index, parts, sim);
	return true;
}

bool AceticAcidKetonisation(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_ACTA || parts[i].temp < 650.0f
		|| IsTouched(i, parts, sim))
		return false;
	auto second = FindLocal(x, y, PT_ACTA, { i }, parts, pmap, sim);
	auto catalyst = FindLocal(x, y, PT_CATA, { i, second.index }, parts, pmap, sim);
	auto empty = FindEmpty(x, y, pmap);
	if (second.index < 0 || catalyst.index < 0 || empty.x < 0
		|| parts[catalyst.index].temp < 650.0f
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(parts, { i, second.index, catalyst.index });
	auto productTemperature = std::min(temperature, 320.0f);
	if (CreateProduct(sim, empty, PT_WATR, parts, productTemperature) < 0)
		return false;
	Convert(sim, { i, x, y }, PT_ACET, parts, productTemperature);
	Convert(sim, second, PT_CO2, parts, productTemperature);
	Touch(i, parts, sim);
	Touch(second.index, parts, sim);
	return true;
}

bool BenzeneMethylation(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_BENZ || parts[i].temp < 500.0f
		|| IsTouched(i, parts, sim))
		return false;
	auto methanol = FindLocal(x, y, PT_METH, { i }, parts, pmap, sim);
	auto catalyst = FindLocal(x, y, PT_CATA, { i, methanol.index }, parts, pmap, sim);
	if (methanol.index < 0 || catalyst.index < 0
		|| parts[catalyst.index].temp < 500.0f
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(parts, { i, methanol.index, catalyst.index });
	auto productTemperature = std::min(temperature, 360.0f);
	Convert(sim, { i, x, y }, PT_TOLU, parts, productTemperature);
	Convert(sim, methanol, PT_WATR, parts, productTemperature);
	Touch(i, parts, sim);
	Touch(methanol.index, parts, sim);
	return true;
}

bool GlycerolNitrationProxy(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_GLYC || parts[i].temp < 330.0f
		|| parts[i].temp > 450.0f || IsTouched(i, parts, sim))
		return false;
	auto nitricAcid = FindLocal(x, y, PT_NITA, { i }, parts, pmap, sim);
	if (nitricAcid.index < 0 || !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = std::min(
		ReactionTemperature(parts, { i, nitricAcid.index }), 350.0f);
	Convert(sim, { i, x, y }, PT_NITR, parts, temperature);
	Convert(sim, nitricAcid, PT_WATR, parts, temperature);
	Touch(i, parts, sim);
	Touch(nitricAcid.index, parts, sim);
	return true;
}

bool UreaHydrolysis(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_UREA || parts[i].temp < 330.0f
		|| parts[i].temp > 500.0f || IsTouched(i, parts, sim))
		return false;
	auto water = FindLocal(x, y, PT_WATR, { i }, parts, pmap, sim);
	auto catalyst = FindLocal(x, y, PT_CATA, { i, water.index }, parts, pmap, sim);
	if (water.index < 0 || catalyst.index < 0
		|| !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = std::min(
		ReactionTemperature(parts, { i, water.index, catalyst.index }), 350.0f);
	Convert(sim, { i, x, y }, PT_AMWA, parts, temperature);
	Convert(sim, water, PT_CO2, parts, temperature);
	Touch(i, parts, sim);
	Touch(water.index, parts, sim);
	return true;
}

bool FatSaponification(UPDATE_FUNC_ARGS)
{
	if (parts[i].type != PT_FATS || parts[i].temp < 330.0f
		|| parts[i].temp > 450.0f || IsTouched(i, parts, sim))
		return false;
	auto base = FindLocal(x, y, PT_CAUS, { i }, parts, pmap, sim);
	if (base.index < 0 || !OmniConsumeChemistryEvent(sim))
		return false;
	auto temperature = ReactionTemperature(parts, { i, base.index });
	Convert(sim, { i, x, y }, PT_SOAP, parts, temperature);
	Convert(sim, base, PT_GLYC, parts, temperature);
	Touch(i, parts, sim);
	Touch(base.index, parts, sim);
	return true;
}

void ConfigureBase(Element &element, RGB colour)
{
	element.Colour = colour;
	element.MenuVisible = 1;
	element.Enabled = 1;
	element.Explosive = 0;
	element.Meltable = 0;
	element.LowPressure = IPL;
	element.LowPressureTransition = NT;
	element.HighPressure = IPH;
	element.HighPressureTransition = NT;
	element.LowTemperature = ITL;
	element.LowTemperatureTransition = NT;
	element.HighTemperature = ITH;
	element.HighTemperatureTransition = NT;
	element.DefaultProperties.temp = 293.15f;
	element.Update = &OmniOrganicElementUpdate;
}

void ConfigureGas(
	Element &element,
	RGB colour,
	float diffusion,
	int weight,
	int flammable,
	int explosive)
{
	ConfigureBase(element, colour);
	element.MenuSection = SC_GAS;
	element.Advection = 1.2f;
	element.AirDrag = 0.01f * CFDS;
	element.AirLoss = 0.99f;
	element.Loss = 0.45f;
	element.Collision = -0.1f;
	element.Gravity = -0.01f;
	element.Diffusion = diffusion;
	element.HotAir = 0.001f * CFDS;
	element.Falldown = 0;
	element.Flammable = flammable;
	element.Explosive = explosive;
	element.Hardness = 0;
	element.Weight = weight;
	element.HeatConduct = 120;
	element.HeatCapacity = 1.40f;
	element.Properties = TYPE_GAS | PROP_NEUTPASS;
}

void ConfigureLiquid(
	Element &element,
	RGB colour,
	int weight,
	int flammable,
	float boilingPoint,
	int properties = TYPE_LIQUID)
{
	ConfigureBase(element, colour);
	element.MenuSection = SC_LIQUID;
	element.Advection = 0.55f;
	element.AirDrag = 0.01f * CFDS;
	element.AirLoss = 0.98f;
	element.Loss = 0.95f;
	element.Collision = 0.0f;
	element.Gravity = 0.1f;
	element.Diffusion = 0.0f;
	element.HotAir = 0.0f * CFDS;
	element.Falldown = 2;
	element.Flammable = flammable;
	element.Hardness = 8;
	element.Weight = weight;
	element.HeatConduct = 70;
	element.HeatCapacity = 1.60f;
	element.Properties = properties;
	if (boilingPoint > 0.0f)
	{
		element.HighTemperature = boilingPoint;
		element.HighTemperatureTransition = PT_GAS;
	}
}

void ConfigurePowder(
	Element &element,
	RGB colour,
	int weight,
	int flammable,
	float decompositionPoint,
	int product)
{
	ConfigureBase(element, colour);
	element.MenuSection = SC_POWDERS;
	element.Advection = 0.0f;
	element.AirDrag = 0.01f * CFDS;
	element.AirLoss = 0.96f;
	element.Loss = 0.90f;
	element.Collision = -0.1f;
	element.Gravity = 0.14f;
	element.Diffusion = 0.0f;
	element.HotAir = 0.0f * CFDS;
	element.Falldown = 1;
	element.Flammable = flammable;
	element.Hardness = 12;
	element.Weight = weight;
	element.HeatConduct = 45;
	element.HeatCapacity = 1.50f;
	element.Properties = TYPE_PART | PROP_NEUTPASS;
	element.HighTemperature = decompositionPoint;
	element.HighTemperatureTransition = product;
}
}

void OmniConfigureOrganicElement(Element &element, int type)
{
	switch (type)
	{
	case PT_CH4M:
		ConfigureGas(element, RGB::Unpack(0xC9E9FF), 2.4f, 2, 1000, 1);
		break;
	case PT_ETHA:
		ConfigureGas(element, RGB::Unpack(0xBDDDF3), 1.9f, 4, 900, 1);
		break;
	case PT_PROP:
		ConfigureGas(element, RGB::Unpack(0xAECEE8), 1.4f, 6, 850, 1);
		element.HighPressure = 8.0f;
		element.HighPressureTransition = PT_OIL;
		break;
	case PT_BUTA:
		ConfigureGas(element, RGB::Unpack(0x9FBFDC), 1.0f, 8, 800, 1);
		element.HighPressure = 5.0f;
		element.HighPressureTransition = PT_OIL;
		break;
	case PT_ETHE:
		ConfigureGas(element, RGB::Unpack(0x91D9C9), 1.6f, 4, 900, 1);
		break;
	case PT_METH:
		ConfigureLiquid(element, RGB::Unpack(0xD8F4FF), 24, 650, 338.0f, TYPE_LIQUID | PROP_DEADLY);
		break;
	case PT_ACET:
		ConfigureLiquid(element, RGB::Unpack(0xD9E5C7), 30, 750, 329.0f, TYPE_LIQUID | PROP_DEADLY);
		break;
	case PT_BENZ:
		ConfigureLiquid(element, RGB::Unpack(0xD8D09B), 36, 650, 353.0f, TYPE_LIQUID | PROP_DEADLY);
		break;
	case PT_TOLU:
		ConfigureLiquid(element, RGB::Unpack(0xC4B67E), 40, 600, 384.0f, TYPE_LIQUID | PROP_DEADLY);
		break;
	case PT_GLYC:
		ConfigureLiquid(element, RGB::Unpack(0xE7E1C8), 48, 180, 0.0f);
		element.HighTemperature = 563.0f;
		element.HighTemperatureTransition = PT_SMKE;
		element.HeatCapacity = 2.40f;
		break;
	case PT_ACTA:
		ConfigureLiquid(element, RGB::Unpack(0xE4F0D0), 44, 90, 391.0f, TYPE_LIQUID | PROP_DEADLY);
		break;
	case PT_UREA:
		ConfigurePowder(element, RGB::Unpack(0xE8E6B0), 55, 20, 406.0f, PT_AMON);
		break;
	case PT_FATS:
		ConfigureLiquid(element, RGB::Unpack(0xE6C878), 60, 80, 0.0f);
		element.HighTemperature = 620.0f;
		element.HighTemperatureTransition = PT_SMKE;
		element.Collision = 0.05f;
		element.Loss = 0.98f;
		break;
	default:
		break;
	}
}

int OmniOrganicElementUpdate(UPDATE_FUNC_ARGS)
{
	if (!OmniChemistryModuleEnabled(sim))
		return 0;
	if (MethaneSynthesis(UPDATE_FUNC_SUBCALL_ARGS)
		|| AcetyleneCyclisation(UPDATE_FUNC_SUBCALL_ARGS)
		|| UreaSynthesis(UPDATE_FUNC_SUBCALL_ARGS)
		|| MethaneSteamReforming(UPDATE_FUNC_SUBCALL_ARGS)
		|| CrackHydrocarbon(UPDATE_FUNC_SUBCALL_ARGS, PT_ETHA, PT_H2, 700.0f)
		|| CrackHydrocarbon(UPDATE_FUNC_SUBCALL_ARGS, PT_PROP, PT_CH4M, 750.0f)
		|| CrackHydrocarbon(UPDATE_FUNC_SUBCALL_ARGS, PT_BUTA, PT_ETHA, 800.0f)
		|| EthylenePolymerisation(UPDATE_FUNC_SUBCALL_ARGS)
		|| AlcoholOxidation(UPDATE_FUNC_SUBCALL_ARGS, PT_METH, PT_COMO, 450.0f, 700.0f)
		|| AlcoholOxidation(UPDATE_FUNC_SUBCALL_ARGS, PT_ETHL, PT_ACTA, 330.0f, 520.0f)
		|| AceticAcidKetonisation(UPDATE_FUNC_SUBCALL_ARGS)
		|| BenzeneMethylation(UPDATE_FUNC_SUBCALL_ARGS)
		|| GlycerolNitrationProxy(UPDATE_FUNC_SUBCALL_ARGS)
		|| UreaHydrolysis(UPDATE_FUNC_SUBCALL_ARGS)
		|| FatSaponification(UPDATE_FUNC_SUBCALL_ARGS))
		return 1;
	return 0;
}
