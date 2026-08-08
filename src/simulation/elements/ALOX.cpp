#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_ALOX()
{
	Identifier = "OMNI_PT_ALOX";
	Name = "ALOX";
	Colour = 0xD9DDE2_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.35f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.88f;
	Collision = -0.1f;
	Gravity = 0.23f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 38;
	Weight = 84;
	HeatConduct = 18;
	HeatCapacity = 0.90f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ALOX");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2345.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
