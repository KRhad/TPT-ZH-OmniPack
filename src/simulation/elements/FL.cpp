#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_FL()
{
	Identifier = "OMNI_PT_FL";
	Name = "FLER";
	Colour = 0xB45F86_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.33f;
	Diffusion = 0.0f;
	HotAir = 0.0003f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;
	Weight = 100;
	HeatConduct = 9;
	HeatCapacity = 0.09f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FL");
	Properties = TYPE_PART | PROP_RADIOACTIVE | PROP_DEADLY | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &OmniCarbonGroupUpdate;
	Graphics = &OmniCarbonGroupGraphics;
	Create = &OmniCarbonGroupCreate;
}
