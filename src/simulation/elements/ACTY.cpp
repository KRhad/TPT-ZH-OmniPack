#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_ACTY()
{
	Identifier = "OMNI_PT_ACTY";
	Name = "ACTY";
	Colour = 0xF6E74C_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Advection = 1.1f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.02f;
	Diffusion = 0.80f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;
	Flammable = 115;
	Explosive = 1;
	Meltable = 0;
	Hardness = 0;
	Weight = 8;
	HeatConduct = 42;
	HeatCapacity = 1.30f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ACTY");
	Properties = TYPE_GAS | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniChemistryElementUpdate;
	Graphics = &OmniGasGraphics;
}
