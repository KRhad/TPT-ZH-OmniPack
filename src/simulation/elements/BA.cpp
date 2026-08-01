#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_BA()
{
	Identifier = "OMNI_PT_BA";
	Name = "BARI";
	Colour = 0xB8C2A6_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.27f;
	Diffusion = 0.0f;
	HotAir = 0.0001f * CFDS;
	Falldown = 1;

	Flammable = 360;
	Explosive = 1;
	Meltable = 5;
	Hardness = 10;
	Weight = 75;
	HeatConduct = 18;
	HeatCapacity = 0.20f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BA");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1000.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkalineEarthMetalUpdate;
	Create = &OmniAlkalineEarthMetalCreate;
}
