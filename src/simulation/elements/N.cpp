#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_N()
{
	Identifier = "OMNI_PT_N";
	Name = "N";
	Colour = 0x8E7CC3_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.0f;
	AirDrag = 0.0f;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = 0.0f;
	Diffusion = 1.10f;
	HotAir = 0.0f;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 3;
	HeatConduct = 15;
	HeatCapacity = 1.04f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_N");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 77.0f;
	LowTemperatureTransition = PT_LNTG;
	HighTemperature = 5000.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNitrogenGroupUpdate;
	Graphics = &OmniNitrogenGroupGraphics;
	Create = &OmniNitrogenGroupCreate;
}
