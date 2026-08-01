#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniIsotopes.h"

void Element::Element_C14I()
{
	Identifier = "OMNI_PT_C14I";
	Name = "C14";
	Colour = 0x4A4A4A_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Advection = 0.35f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.86f;
	Collision = -0.1f;
	Gravity = 0.28f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 2;
	Weight = 80;
	HeatConduct = 90;
	HeatCapacity = 0.70f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_C14I");
	Properties = TYPE_PART | PROP_LIFE_DEC | PROP_RADIOACTIVE | PROP_CONDUCTS | PROP_HOT_GLOW;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 4000.0f;
	HighTemperatureTransition = NT;
	Update = &OmniIsotopeElementUpdate;
	Graphics = &OmniIsotopeGraphics;
	Create = &OmniIsotopeCreate;
}
