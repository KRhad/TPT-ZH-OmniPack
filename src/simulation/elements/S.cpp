#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_S()
{
	Identifier = "OMNI_PT_S";
	Name = "S";
	Colour = 0xE6D335_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.26f;
	Diffusion = 0.0f;
	HotAir = 0.0005f * CFDS;
	Falldown = 1;

	Flammable = 35;
	Explosive = 0;
	Meltable = 1;
	Hardness = 2;
	Weight = 42;
	HeatConduct = 35;
	HeatCapacity = 0.71f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_S");
	Properties = TYPE_PART | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 388.4f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniOxygenGroupUpdate;
	Graphics = &OmniOxygenGroupGraphics;
	Create = &OmniOxygenGroupCreate;
}
