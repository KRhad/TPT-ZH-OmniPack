#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_FESF()
{
	Identifier = "OMNI_PT_FESF";
	Name = "FESF";
	Colour = 0x5D5541_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.37f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.87f;
	Collision = -0.1f;
	Gravity = 0.25f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 22;
	Weight = 92;
	HeatConduct = 23;
	HeatCapacity = 0.75f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FESF");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1460.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
