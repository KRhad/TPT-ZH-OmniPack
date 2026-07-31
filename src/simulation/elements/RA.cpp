#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_RA()
{
	Identifier = "OMNI_PT_RA";
	Name = "RA";
	Colour = 0xA8CFA4_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.29f;
	Diffusion = 0.0f;
	HotAir = 0.0002f * CFDS;
	Falldown = 1;

	Flammable = 420;
	Explosive = 1;
	Meltable = 4;
	Hardness = 8;
	Weight = 95;
	HeatConduct = 18;
	HeatCapacity = 0.12f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_RA");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW |
		PROP_RADIOACTIVE | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 973.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkalineEarthMetalUpdate;
	Create = &OmniAlkalineEarthMetalCreate;
}
