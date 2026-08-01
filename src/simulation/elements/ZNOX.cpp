#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_ZNOX()
{
	Identifier = "OMNI_PT_ZNOX";
	Name = "ZNOX";
	Colour = 0xEEE9D0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.39f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.85f;
	Collision = -0.1f;
	Gravity = 0.23f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 22;
	Weight = 82;
	HeatConduct = 25;
	HeatCapacity = 0.82f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ZNOX");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2248.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
