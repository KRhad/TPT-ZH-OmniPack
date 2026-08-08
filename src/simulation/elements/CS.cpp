#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_CS()
{
	Identifier = "OMNI_PT_CS";
	Name = "CAES";
	Colour = 0xD7B56D_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.28f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;

	Flammable = 700;
	Explosive = 1;
	Meltable = 5;
	Hardness = 3;
	Weight = 82;
	HeatConduct = 150;
	HeatCapacity = 0.68f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CS");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 301.59f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkaliMetalUpdate;
	Create = &OmniAlkaliMetalCreate;
}
