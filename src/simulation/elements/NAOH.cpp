#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_NAOH()
{
	Identifier = "OMNI_PT_NAOH";
	Name = "NAOH";
	Colour = 0xF4F4F0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.45f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.17f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 12;
	Weight = 55;
	HeatConduct = 45;
	HeatCapacity = 1.40f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NAOH");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 590.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
