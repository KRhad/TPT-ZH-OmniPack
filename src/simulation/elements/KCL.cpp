#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_KCL()
{
	Identifier = "OMNI_PT_KCL";
	Name = "PTCL";
	Colour = 0xE9E4F0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.46f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.19f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 12;
	Weight = 64;
	HeatConduct = 28;
	HeatCapacity = 0.95f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_KCL");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1043.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
