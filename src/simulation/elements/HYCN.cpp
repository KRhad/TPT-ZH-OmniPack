#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_HYCN()
{
	Identifier = "OMNI_PT_HYCN";
	Name = "HYCN";
	Colour = 0xC7D8DB_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Advection = 1.12f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.02f;
	Diffusion = 0.90f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;
	Flammable = 25;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 14;
	HeatConduct = 44;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_HYCN");
	Properties = TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniInorganicElementUpdate;
	Graphics = &OmniGasGraphics;
}
