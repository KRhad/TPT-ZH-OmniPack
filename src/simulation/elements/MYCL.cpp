#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_MYCL()
{
	Identifier = "OMNI_PT_MYCL";
	Name = "MYCL";
	Colour = 0xD8D2B8_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;
	Flammable = 4;
	Explosive = 0;
	Meltable = 0;
	Hardness = 6;
	Weight = 72;
	HeatConduct = 45;
	HeatCapacity = 1.25f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_MYCL");
	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 560.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniBiologyElementUpdate;
}
