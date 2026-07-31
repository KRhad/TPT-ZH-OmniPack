#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_SE()
{
	Identifier = "OMNI_PT_SE";
	Name = "SE";
	Colour = 0x8B4C45_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.35f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.28f;
	Diffusion = 0.0f;
	HotAir = 0.0003f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 12;
	Weight = 65;
	HeatConduct = 22;
	HeatCapacity = 0.32f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SE");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_DEADLY |
		PROP_HOT_GLOW | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 494.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniOxygenGroupUpdate;
	Graphics = &OmniOxygenGroupGraphics;
	Create = &OmniOxygenGroupCreate;
}
