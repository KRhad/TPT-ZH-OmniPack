#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMetallurgy.h"

void Element::Element_ALMG()
{
	Identifier = "OMNI_PT_ALMG";
	Name = "ALMG";
	Colour = 0xBAC3C3_rgb;
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
	Hardness = 18;
	Weight = 50;
	HeatConduct = 205;
	HeatCapacity = 0.85f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ALMG");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 880.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniMetallurgyMetalUpdate;
}
