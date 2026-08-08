#include "simulation/ElementCommon.h"
#include "common/Localization.h"

void Element::Element_CRUC()
{
	Identifier = "OMNI_PT_CRUC";
	Name = "CRUC";
	Colour = 0x62584C_rgb;
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
	Meltable = 0;
	Hardness = 0;
	Weight = 120;
	HeatConduct = 25;
	HeatCapacity = 1.60f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CRUC");
	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 180.0f;
	HighPressureTransition = PT_BRCK;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3200.0f;
	HighTemperatureTransition = PT_LAVA;
}
