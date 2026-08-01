#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_KOH()
{
	Identifier = "OMNI_PT_KOH";
	Name = "KOH";
	Colour = 0xEEEDE4_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.48f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.81f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 10;
	Weight = 62;
	HeatConduct = 41;
	HeatCapacity = 1.25f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_KOH");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 630.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
