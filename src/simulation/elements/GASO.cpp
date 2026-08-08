#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_GASO()
{
	Identifier = "OMNI_PT_GASO";
	Name = "GASO";
	Colour = 0xE6A530_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.75f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.08f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 72;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;
	Weight = 18;
	HeatConduct = 30;
	HeatCapacity = 2.20f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_GASO");
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 180.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniChemistryElementUpdate;
}
