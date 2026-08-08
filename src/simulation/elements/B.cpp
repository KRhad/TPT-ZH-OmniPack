#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_B()
{
	Identifier = "OMNI_PT_B";
	Name = "BORO";
	Colour = 0x5B4637_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.10f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.90f;
	Collision = -0.1f;
	Gravity = 0.20f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 80;
	Weight = 25;
	HeatConduct = 27;
	HeatCapacity = 1.03f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_B");
	Properties = TYPE_PART | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2349.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniBoronGroupUpdate;
	Create = &OmniBoronGroupCreate;
}
