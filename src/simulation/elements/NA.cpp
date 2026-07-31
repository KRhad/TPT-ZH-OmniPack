#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_NA()
{
	Identifier = "OMNI_PT_NA";
	Name = "NA";
	Colour = 0xD8D9DC_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.23f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;

	Flammable = 180;
	Explosive = 0;
	Meltable = 10;
	Hardness = 8;
	Weight = 24;
	HeatConduct = 205;
	HeatCapacity = 0.95f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NA");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 370.87f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkaliMetalUpdate;
	Create = &OmniAlkaliMetalCreate;
}
