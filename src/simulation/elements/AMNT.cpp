#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_AMNT()
{
	Identifier = "OMNI_PT_AMNT";
	Name = "AMNT";
	Colour = 0xF2F0E8_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.48f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 9;
	Weight = 62;
	HeatConduct = 24;
	HeatCapacity = 1.20f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AMNT");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniInorganicElementUpdate;
}
