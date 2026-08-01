#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_TIOX()
{
	Identifier = "OMNI_PT_TIOX";
	Name = "TIOX";
	Colour = 0xF1F2EA_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.34f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.88f;
	Collision = -0.1f;
	Gravity = 0.24f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 34;
	Weight = 82;
	HeatConduct = 16;
	HeatCapacity = 0.88f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_TIOX");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2116.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
