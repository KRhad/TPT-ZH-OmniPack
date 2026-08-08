#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniNuclear.h"

void Element::Element_NCLT()
{
	Identifier = "OMNI_PT_NCLT";
	Name = "NCLT";
	Colour = 0x5EC8D9_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Advection = 0.75f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;
	Weight = 32;
	HeatConduct = 180;
	HeatCapacity = 2.00f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NCLT");
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 220.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 1200.0f;
	HighTemperatureTransition = PT_WTRV;
	Update = &OmniNuclearElementUpdate;
}
