#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_HUMS()
{
	Identifier = "OMNI_PT_HUMS";
	Name = "HUMS";
	Colour = 0x76563B_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.28f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.20f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 1;
	Explosive = 0;
	Meltable = 0;
	Hardness = 8;
	Weight = 66;
	HeatConduct = 39;
	HeatCapacity = 1.30f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_HUMS");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 640.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniBiologyElementUpdate;
}
