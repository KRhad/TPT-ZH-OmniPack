#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_K()
{
	Identifier = "OMNI_PT_K";
	Name = "POTA";
	Colour = 0xC8BBD7_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.25f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;

	Flammable = 320;
	Explosive = 0;
	Meltable = 8;
	Hardness = 6;
	Weight = 39;
	HeatConduct = 185;
	HeatCapacity = 0.82f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_K");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 336.53f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkaliMetalUpdate;
	Create = &OmniAlkaliMetalCreate;
}
