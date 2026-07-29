#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMetallurgy.h"

void Element::Element_COPR()
{
	Identifier = "OMNI_PT_COPR";
	Name = "COPR";
	Colour = 0xB87333_rgb;
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
	Hardness = 10;
	Weight = 90;
	HeatConduct = 255;
	HeatCapacity = 1.25f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_COPR");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1357.77f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniMetallurgyMetalUpdate;
}
