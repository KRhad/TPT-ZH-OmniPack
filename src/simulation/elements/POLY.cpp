#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_POLY()
{
	Identifier = "OMNI_PT_POLY";
	Name = "POLY";
	Colour = 0xEDE7DD_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;
	Flammable = 22;
	Explosive = 0;
	Meltable = 0;
	Hardness = 45;
	Weight = 90;
	HeatConduct = 10;
	HeatCapacity = 1.80f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_POLY");
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 900.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniChemistryElementUpdate;
}
