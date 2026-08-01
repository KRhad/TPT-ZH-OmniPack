#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_UROX()
{
	Identifier = "OMNI_PT_UROX";
	Name = "UROX";
	Colour = 0x526B31_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Advection = 0.32f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.89f;
	Collision = -0.1f;
	Gravity = 0.30f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 28;
	Weight = 126;
	HeatConduct = 18;
	HeatCapacity = 0.55f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_UROX");
	Properties = TYPE_PART | PROP_DEADLY | PROP_RADIOACTIVE | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3120.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
