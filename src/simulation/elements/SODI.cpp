#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_SODI()
{
	Identifier = "OMNI_PT_SODI";
	Name = "SODI";
	Colour = 0xD4D8DE_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Advection = 1.10f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.05f;
	Diffusion = 0.68f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 30;
	HeatConduct = 44;
	HeatCapacity = 0.82f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SODI");
	Properties = TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &OmniInorganicElementUpdate;
}
