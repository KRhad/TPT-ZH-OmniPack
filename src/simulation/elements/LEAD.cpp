#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMetallurgy.h"

void Element::Element_LEAD()
{
	Identifier = "OMNI_PT_LEAD";
	Name = "LEAD";
	Colour = 0x59616B_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 6;
	Weight = 145;
	HeatConduct = 80;
	HeatCapacity = 0.55f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_LEAD");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTABSORB | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 600.61f;
	HighTemperatureTransition = PT_LAVA;

	Update = &OmniMetallurgyMetalUpdate;
}
