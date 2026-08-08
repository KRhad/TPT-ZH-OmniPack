#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_SPOR()
{
	Identifier = "OMNI_PT_SPOR";
	Name = "SPOR";
	Colour = 0xB88FC7_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.65f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.85f;
	Collision = -0.05f;
	Gravity = 0.09f;
	Diffusion = 0.16f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 2;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;
	Weight = 18;
	HeatConduct = 28;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SPOR");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 480.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniBiologyElementUpdate;
}
