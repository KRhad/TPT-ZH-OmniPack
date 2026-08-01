#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_FEOX()
{
	Identifier = "OMNI_PT_FEOX";
	Name = "FEOX";
	Colour = 0x9B4E31_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.38f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.86f;
	Collision = -0.1f;
	Gravity = 0.24f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 28;
	Weight = 90;
	HeatConduct = 22;
	HeatCapacity = 0.85f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FEOX");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1838.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
