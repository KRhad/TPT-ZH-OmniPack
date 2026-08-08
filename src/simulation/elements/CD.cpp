#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_CD()
{
	Identifier = "OMNI_PT_CD";
	Name = "CADM";
	Colour = 0xA8B4BC_rgb;
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
	Hardness = 12;
	Weight = 88;
	HeatConduct = 110;
	HeatCapacity = 0.23f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CD");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_DEADLY | PROP_NEUTABSORB | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 594.22f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniSecondTransitionUpdate;
	Graphics = &OmniSecondTransitionGraphics;
	Create = &OmniSecondTransitionCreate;
}
