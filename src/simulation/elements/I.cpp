#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_I()
{
	Identifier = "OMNI_PT_I";
	Name = "I";
	Colour = 0x4B315F_rgb;
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
	Hardness = 8;
	Weight = 70;
	HeatConduct = 10;
	HeatCapacity = 0.21f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_I");
	Properties = TYPE_SOLID | PROP_DEADLY | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 386.85f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniHalogenUpdate;
	Graphics = &OmniHalogenGraphics;
	Create = &OmniHalogenCreate;
}
