#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_SR()
{
	Identifier = "OMNI_PT_SR";
	Name = "SR";
	Colour = 0xC9D0BC_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.24f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;

	Flammable = 240;
	Explosive = 0;
	Meltable = 7;
	Hardness = 15;
	Weight = 55;
	HeatConduct = 35;
	HeatCapacity = 0.30f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SR");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1050.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkalineEarthMetalUpdate;
	Create = &OmniAlkalineEarthMetalCreate;
}
