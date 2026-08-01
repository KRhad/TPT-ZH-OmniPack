#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_COMO()
{
	Identifier = "OMNI_PT_COMO";
	Name = "COMO";
	Colour = 0xA8ADB3_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Advection = 1.05f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.01f;
	Diffusion = 0.80f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 18;
	HeatConduct = 52;
	HeatCapacity = 1.00f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_COMO");
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
