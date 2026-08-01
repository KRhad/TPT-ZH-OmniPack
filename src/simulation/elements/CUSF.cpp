#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CUSF()
{
	Identifier = "OMNI_PT_CUSF";
	Name = "CUSF";
	Colour = 0x3D86D1_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.43f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.83f;
	Collision = -0.1f;
	Gravity = 0.21f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 18;
	Weight = 82;
	HeatConduct = 38;
	HeatCapacity = 0.90f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CUSF");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 900.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
