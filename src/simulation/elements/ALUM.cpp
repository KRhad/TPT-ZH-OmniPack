#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMetallurgy.h"
#include "simulation/OmniPeriodic.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_ALUM()
{
	Identifier = "OMNI_PT_ALUM";
	Name = "ALUM";
	Colour = 0xC8CED4_rgb;
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
	Hardness = 15;
	Weight = 55;
	HeatConduct = 235;
	HeatCapacity = 0.90f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ALUM");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 933.47f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (OmniBoronGroupUpdate(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return OmniMetallurgyMetalUpdate(UPDATE_FUNC_SUBCALL_ARGS);
}
