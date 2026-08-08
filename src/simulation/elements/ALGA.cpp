#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_ALGA()
{
	Identifier = "OMNI_PT_ALGA";
	Name = "ALGA";
	Colour = 0x3DBE73_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.65f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.94f;
	Collision = 0.0f;
	Gravity = 0.04f;
	Diffusion = 0.02f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 8;
	Weight = 38;
	HeatConduct = 72;
	HeatCapacity = 1.30f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ALGA");
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 270.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_WTRV;
	Update = &OmniBiologyElementUpdate;
}
