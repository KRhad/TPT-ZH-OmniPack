#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_MN()
{
	Identifier = "OMNI_PT_MN";
	Name = "MN";
	Colour = 0x9AA0A4_rgb;
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
	Hardness = 55;
	Weight = 72;
	HeatConduct = 78;
	HeatCapacity = 0.48f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_MN");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1519.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniFirstTransitionUpdate;
	Graphics = &OmniFirstTransitionGraphics;
}
