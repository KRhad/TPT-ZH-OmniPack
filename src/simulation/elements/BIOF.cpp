#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_BIOF()
{
	Identifier = "OMNI_PT_BIOF";
	Name = "BIOF";
	Colour = 0x4A938A_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 12;
	Weight = 82;
	HeatConduct = 50;
	HeatCapacity = 1.35f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BIOF");
	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;
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
