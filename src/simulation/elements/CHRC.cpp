#include "simulation/ElementCommon.h"
#include "common/Localization.h"

void Element::Element_CHRC()
{
	Identifier = "OMNI_PT_CHRC";
	Name = "CHRC";
	Colour = 0x2D2822_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.80f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;

	Flammable = 18;
	Explosive = 0;
	Meltable = 0;
	Hardness = 12;
	Weight = 35;
	HeatConduct = 80;
	HeatCapacity = 0.70f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CHRC");
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
