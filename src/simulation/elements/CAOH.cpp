#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CAOH()
{
	Identifier = "OMNI_PT_CAOH";
	Name = "CAOH";
	Colour = 0xE8E5D0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.40f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.84f;
	Collision = -0.1f;
	Gravity = 0.19f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 16;
	Weight = 68;
	HeatConduct = 32;
	HeatCapacity = 1.10f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CAOH");
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
