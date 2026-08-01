#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_ALCL()
{
	Identifier = "OMNI_PT_ALCL";
	Name = "ALCL";
	Colour = 0xE5E3D8_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.43f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.83f;
	Collision = -0.1f;
	Gravity = 0.20f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 12;
	Weight = 76;
	HeatConduct = 24;
	HeatCapacity = 0.92f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ALCL");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 465.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
