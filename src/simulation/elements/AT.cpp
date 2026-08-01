#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_AT()
{
	Identifier = "OMNI_PT_AT";
	Name = "ASTA";
	Colour = 0x4E4057_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.0f;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0002f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 6;
	Weight = 92;
	HeatConduct = 8;
	HeatCapacity = 0.12f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AT");
	Properties = TYPE_SOLID | PROP_RADIOACTIVE | PROP_DEADLY | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 575.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniHalogenUpdate;
	Graphics = &OmniHalogenGraphics;
	Create = &OmniHalogenCreate;
}
