#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CACL()
{
	Identifier = "OMNI_PT_CACL";
	Name = "CACL";
	Colour = 0xF0EFE8_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.44f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.21f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 13;
	Weight = 72;
	HeatConduct = 32;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CACL");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1045.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
