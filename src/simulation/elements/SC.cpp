#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_SC()
{
	Identifier = "OMNI_PT_SC";
	Name = "SC";
	Colour = 0xC3CCD4_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.0f;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 18;
	Weight = 55;
	HeatConduct = 82;
	HeatCapacity = 0.57f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SC");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1814.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniFirstTransitionUpdate;
	Graphics = &OmniFirstTransitionGraphics;
}
