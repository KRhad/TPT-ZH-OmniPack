#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniNuclear.h"

void Element::Element_NGEN()
{
	Identifier = "OMNI_PT_NGEN";
	Name = "NGEN";
	Colour = 0xD1C665_rgb;
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
	Hardness = 40;
	Weight = 100;
	HeatConduct = 170;
	HeatCapacity = 1.55f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NGEN");
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2800.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniNuclearElementUpdate;
}
