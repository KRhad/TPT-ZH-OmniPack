#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_SICB()
{
	Identifier = "OMNI_PT_SICB";
	Name = "SICB";
	Colour = 0x30363A_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Advection = 0.0f;
	AirDrag = 0.0f;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 55;
	Weight = 82;
	HeatConduct = 72;
	HeatCapacity = 0.68f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SICB");
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3000.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
