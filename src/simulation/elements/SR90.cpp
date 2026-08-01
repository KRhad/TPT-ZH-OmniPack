#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniIsotopes.h"

void Element::Element_SR90()
{
	Identifier = "OMNI_PT_SR90";
	Name = "SR90";
	Colour = 0xD8C878_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
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
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 5;
	Weight = 98;
	HeatConduct = 35;
	HeatCapacity = 0.31f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SR90");
	Properties = TYPE_SOLID | PROP_LIFE_DEC | PROP_RADIOACTIVE | PROP_CONDUCTS | PROP_HOT_GLOW;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1050.0f;
	HighTemperatureTransition = NT;
	Update = &OmniIsotopeElementUpdate;
	Graphics = &OmniIsotopeGraphics;
	Create = &OmniIsotopeCreate;
}
