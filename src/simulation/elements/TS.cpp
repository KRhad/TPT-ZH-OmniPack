#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_TS()
{
	Identifier = "OMNI_PT_TS";
	Name = "TS";
	Colour = 0xD84663_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.38f;
	Diffusion = 0.0f;
	HotAir = 0.0004f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;
	Weight = 100;
	HeatConduct = 6;
	HeatCapacity = 0.06f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_TS");
	Properties = TYPE_PART | PROP_RADIOACTIVE | PROP_DEADLY | PROP_HOT_GLOW;

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
