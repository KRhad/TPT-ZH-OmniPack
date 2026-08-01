#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_FR()
{
	Identifier = "OMNI_PT_FR";
	Name = "FRAN";
	Colour = 0xA96B8F_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.30f;
	Diffusion = 0.0f;
	HotAir = 0.0002f * CFDS;
	Falldown = 1;

	Flammable = 1000;
	Explosive = 1;
	Meltable = 3;
	Hardness = 2;
	Weight = 100;
	HeatConduct = 120;
	HeatCapacity = 0.60f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FR");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_HOT_GLOW |
		PROP_RADIOACTIVE | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 300.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniAlkaliMetalUpdate;
	Create = &OmniAlkaliMetalCreate;
}
