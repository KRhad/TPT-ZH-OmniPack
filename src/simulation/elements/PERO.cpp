#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_PERO()
{
	Identifier = "OMNI_PT_PERO";
	Name = "PERO";
	Colour = 0xA9D8FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.62f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.10f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 8;
	Weight = 31;
	HeatConduct = 35;
	HeatCapacity = 2.60f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PERO");
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 270.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 620.0f;
	HighTemperatureTransition = PT_WTRV;
	Update = &OmniChemistryElementUpdate;
}
