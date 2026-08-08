#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_HYFA()
{
	Identifier = "OMNI_PT_HYFA";
	Name = "HYFA";
	Colour = 0xB9F3E8_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.68f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.94f;
	Collision = 0.0f;
	Gravity = 0.09f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 3;
	Weight = 38;
	HeatConduct = 40;
	HeatCapacity = 2.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_HYFA");
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 240.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 390.0f;
	HighTemperatureTransition = PT_CAUS;
	Update = &OmniInorganicElementUpdate;
}
