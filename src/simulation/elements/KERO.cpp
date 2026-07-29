#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_KERO()
{
	Identifier = "OMNI_PT_KERO";
	Name = "KERO";
	Colour = 0x6B5129_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.68f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.10f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 38;
	Explosive = 0;
	Meltable = 0;
	Hardness = 3;
	Weight = 22;
	HeatConduct = 39;
	HeatCapacity = 2.10f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_KERO");
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 210.0f;
	LowTemperatureTransition = PT_WAX;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniChemistryElementUpdate;
}
