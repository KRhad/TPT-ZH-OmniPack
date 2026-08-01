#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_NASD()
{
	Identifier = "OMNI_PT_NASD";
	Name = "NASD";
	Colour = 0xE6DFC0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.45f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.19f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 11;
	Weight = 65;
	HeatConduct = 27;
	HeatCapacity = 1.10f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NASD");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1450.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
