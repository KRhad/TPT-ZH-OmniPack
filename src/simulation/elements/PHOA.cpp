#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_PHOA()
{
	Identifier = "OMNI_PT_PHOA";
	Name = "PHOA";
	Colour = 0xEFEBD0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.38f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.97f;
	Collision = 0.0f;
	Gravity = 0.12f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;
	Weight = 60;
	HeatConduct = 25;
	HeatCapacity = 1.55f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PHOA");
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 275.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 700.0f;
	HighTemperatureTransition = PT_CAUS;
	Update = &OmniInorganicElementUpdate;
}
