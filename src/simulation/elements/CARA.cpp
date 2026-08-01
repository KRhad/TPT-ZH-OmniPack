#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CARA()
{
	Identifier = "OMNI_PT_CARA";
	Name = "CARA";
	Colour = 0xC8E8F0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.70f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.09f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 2;
	Weight = 40;
	HeatConduct = 34;
	HeatCapacity = 2.10f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CARA");
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 260.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniInorganicElementUpdate;
}
