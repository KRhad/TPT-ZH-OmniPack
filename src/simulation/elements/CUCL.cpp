#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CUCL()
{
	Identifier = "OMNI_PT_CUCL";
	Name = "CUCL";
	Colour = 0xC7B59A_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.40f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.85f;
	Collision = -0.1f;
	Gravity = 0.23f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 17;
	Weight = 86;
	HeatConduct = 30;
	HeatCapacity = 0.82f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CUCL");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 703.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
