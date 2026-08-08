#include "simulation/ElementCommon.h"
#include "common/Localization.h"

void Element::Element_SLAG()
{
	Identifier = "OMNI_PT_SLAG";
	Name = "SLAG";
	Colour = 0x4C4237_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.20f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.85f;
	Collision = -0.1f;
	Gravity = 0.22f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 2;
	Weight = 75;
	HeatConduct = 70;
	HeatCapacity = 1.25f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SLAG");
	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1450.0f;
	HighTemperatureTransition = PT_LAVA;
}
