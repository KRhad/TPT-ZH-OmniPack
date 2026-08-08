#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMetallurgy.h"
#include "simulation/OmniPeriodic.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_TIN()
{
	Identifier = "OMNI_PT_TIN";
	Name = "TINN";
	Colour = 0xB8C0C8_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 5;
	Weight = 82;
	HeatConduct = 125;
	HeatCapacity = 0.65f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_TIN");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 505.08f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (OmniCarbonGroupUpdate(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return OmniMetallurgyMetalUpdate(UPDATE_FUNC_SUBCALL_ARGS);
}
