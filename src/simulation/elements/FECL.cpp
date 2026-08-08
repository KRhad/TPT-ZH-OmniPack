#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_FECL()
{
	Identifier = "OMNI_PT_FECL";
	Name = "FECL";
	Colour = 0x9A6A38_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.42f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.84f;
	Collision = -0.1f;
	Gravity = 0.23f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 15;
	Weight = 86;
	HeatConduct = 30;
	HeatCapacity = 0.85f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FECL");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 580.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
