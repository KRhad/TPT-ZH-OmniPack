#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_CA()
{
	Identifier = "OMNI_PT_CA";
	Name = "CALC";
	Colour = 0xD8D1B5_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.22f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;

	Flammable = 120;
	Explosive = 0;
	Meltable = 8;
	Hardness = 20;
	Weight = 35;
	HeatConduct = 100;
	HeatCapacity = 0.65f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CA");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1115.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkalineEarthMetalUpdate;
	Create = &OmniAlkalineEarthMetalCreate;
}
