#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniIsotopes.h"

void Element::Element_CF52()
{
	Identifier = "OMNI_PT_CF52";
	Name = "CF52";
	Colour = 0xE0B84F_rgb;
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
	Hardness = 4;
	Weight = 142;
	HeatConduct = 25;
	HeatCapacity = 0.10f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CF52");
	Properties = TYPE_SOLID | PROP_LIFE_DEC | PROP_RADIOACTIVE | PROP_CONDUCTS | PROP_HOT_GLOW;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1173.0f;
	HighTemperatureTransition = NT;
	Update = &OmniIsotopeElementUpdate;
	Graphics = &OmniIsotopeGraphics;
	Create = &OmniIsotopeCreate;
}
