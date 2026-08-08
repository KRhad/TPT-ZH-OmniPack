#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_AMON()
{
	Identifier = "OMNI_PT_AMON";
	Name = "AMON";
	Colour = 0xB8E8FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Advection = 1.3f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.03f;
	Diffusion = 0.85f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;
	Flammable = 8;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 10;
	HeatConduct = 52;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AMON");
	Properties = TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS;
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
