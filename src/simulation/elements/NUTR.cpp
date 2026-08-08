#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_NUTR()
{
	Identifier = "OMNI_PT_NUTR";
	Name = "NUTR";
	Colour = 0xB9A85A_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.35f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.80f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 4;
	Weight = 54;
	HeatConduct = 35;
	HeatCapacity = 1.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NUTR");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 650.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniBiologyElementUpdate;
}
