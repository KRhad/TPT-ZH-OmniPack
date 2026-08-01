#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_BR()
{
	Identifier = "OMNI_PT_BR";
	Name = "BROM";
	Colour = 0x8B2F20_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.12f;
	Diffusion = 0.0f;
	HotAir = 0.0004f * CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 55;
	HeatConduct = 25;
	HeatCapacity = 0.47f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BR");
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &OmniHalogenUpdate;
	Graphics = &OmniHalogenGraphics;
	Create = &OmniHalogenCreate;
}
