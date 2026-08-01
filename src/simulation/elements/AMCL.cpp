#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_AMCL()
{
	Identifier = "OMNI_PT_AMCL";
	Name = "AMCL";
	Colour = 0xEFEDE4_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.46f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.81f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 9;
	Weight = 58;
	HeatConduct = 22;
	HeatCapacity = 1.12f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AMCL");
	Properties = TYPE_PART | PROP_NEUTPASS;
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
