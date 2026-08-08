#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CAOX()
{
	Identifier = "OMNI_PT_CAOX";
	Name = "CAOX";
	Colour = 0xD7D2BC_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.38f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.86f;
	Collision = -0.1f;
	Gravity = 0.22f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 30;
	Weight = 74;
	HeatConduct = 30;
	HeatCapacity = 0.80f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CAOX");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2850.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
