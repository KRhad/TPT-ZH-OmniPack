#include "simulation/ElementCommon.h"
#include "common/Localization.h"

void Element::Element_COKE()
{
	Identifier = "OMNI_PT_COKE";
	Name = "COKE";
	Colour = 0x17191C_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.25f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.85f;
	Collision = -0.1f;
	Gravity = 0.22f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;

	Flammable = 6;
	Explosive = 0;
	Meltable = 0;
	Hardness = 16;
	Weight = 55;
	HeatConduct = 110;
	HeatCapacity = 0.85f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_COKE");
	Properties = TYPE_PART | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
}
