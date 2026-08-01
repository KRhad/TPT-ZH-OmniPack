#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CAHY()
{
	Identifier = "OMNI_PT_CAHY";
	Name = "CAHY";
	Colour = 0xC8CBC2_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.43f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.83f;
	Collision = -0.1f;
	Gravity = 0.19f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 14;
	Explosive = 0;
	Meltable = 1;
	Hardness = 12;
	Weight = 58;
	HeatConduct = 30;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CAHY");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1273.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
