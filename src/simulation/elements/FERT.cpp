#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_FERT()
{
	Identifier = "OMNI_PT_FERT";
	Name = "FERT";
	Colour = 0xB6B65B_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.45f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 10;
	Weight = 58;
	HeatConduct = 42;
	HeatCapacity = 1.20f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FERT");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 650.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniChemistryElementUpdate;
}
