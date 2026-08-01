#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_NAHY()
{
	Identifier = "OMNI_PT_NAHY";
	Name = "NAHY";
	Colour = 0xD7D9D0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.46f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.81f;
	Collision = -0.1f;
	Gravity = 0.16f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 18;
	Explosive = 0;
	Meltable = 1;
	Hardness = 8;
	Weight = 42;
	HeatConduct = 34;
	HeatCapacity = 1.20f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NAHY");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1073.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
