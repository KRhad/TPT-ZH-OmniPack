#include "simulation/ElementCommon.h"
#include "common/Localization.h"

void Element::Element_FLUX()
{
	Identifier = "OMNI_PT_FLUX";
	Name = "FLUX";
	Colour = 0xD9D0B6_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.35f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = -0.1f;
	Gravity = 0.20f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 4;
	Weight = 62;
	HeatConduct = 55;
	HeatCapacity = 1.10f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FLUX");
	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1050.0f;
	HighTemperatureTransition = PT_LAVA;
}
