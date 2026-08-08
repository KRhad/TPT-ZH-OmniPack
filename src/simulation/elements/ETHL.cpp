#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"

void Element::Element_ETHL()
{
	Identifier = "OMNI_PT_ETHL";
	Name = "ETHL";
	Colour = 0xD6F2F7_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.72f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.09f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 48;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;
	Weight = 24;
	HeatConduct = 33;
	HeatCapacity = 2.40f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ETHL");
	Properties = TYPE_LIQUID;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 159.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniChemistryElementUpdate;
}
