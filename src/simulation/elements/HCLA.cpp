#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_HCLA()
{
	Identifier = "OMNI_PT_HCLA";
	Name = "HCLA";
	Colour = 0xBDEBFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.65f;
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
	Hardness = 4;
	Weight = 42;
	HeatConduct = 36;
	HeatCapacity = 2.20f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_HCLA");
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 245.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 390.0f;
	HighTemperatureTransition = PT_CAUS;
	Update = &OmniInorganicElementUpdate;
}
