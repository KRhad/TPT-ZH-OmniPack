#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_SULA()
{
	Identifier = "OMNI_PT_SULA";
	Name = "SULA";
	Colour = 0xE8E6D5_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.42f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.96f;
	Collision = 0.0f;
	Gravity = 0.13f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 6;
	Weight = 68;
	HeatConduct = 28;
	HeatCapacity = 1.45f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SULA");
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 270.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 620.0f;
	HighTemperatureTransition = PT_CAUS;
	Update = &OmniInorganicElementUpdate;
}
