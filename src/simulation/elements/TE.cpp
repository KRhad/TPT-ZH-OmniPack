#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_TE()
{
	Identifier = "OMNI_PT_TE";
	Name = "TE";
	Colour = 0x8F9699_rgb;
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
	Hardness = 24;
	Weight = 82;
	HeatConduct = 28;
	HeatCapacity = 0.20f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_TE");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_DEADLY | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 722.7f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniOxygenGroupUpdate;
	Graphics = &OmniOxygenGroupGraphics;
	Create = &OmniOxygenGroupCreate;
}
