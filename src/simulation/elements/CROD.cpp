#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniNuclear.h"

void Element::Element_CROD()
{
	Identifier = "OMNI_PT_CROD";
	Name = "CROD";
	Colour = 0x2A3549_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;
	Weight = 96;
	HeatConduct = 140;
	HeatCapacity = 1.60f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CROD");
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3200.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniNuclearElementUpdate;
}
