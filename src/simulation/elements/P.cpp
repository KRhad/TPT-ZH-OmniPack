#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_P()
{
	Identifier = "OMNI_PT_P";
	Name = "PHOS";
	Colour = 0xEADCBF_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.30f;
	Diffusion = 0.0f;
	HotAir = 0.0004f * CFDS;
	Falldown = 1;

	Flammable = 30;
	Explosive = 0;
	Meltable = 1;
	Hardness = 2;
	Weight = 55;
	HeatConduct = 30;
	HeatCapacity = 0.77f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_P");
	Properties = TYPE_PART | PROP_DEADLY | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 317.3f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniNitrogenGroupUpdate;
	Graphics = &OmniNitrogenGroupGraphics;
	Create = &OmniNitrogenGroupCreate;
}
