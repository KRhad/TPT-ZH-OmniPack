#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CAPH()
{
	Identifier = "OMNI_PT_CAPH";
	Name = "CAPH";
	Colour = 0xE8E4D2_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.42f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.84f;
	Collision = -0.1f;
	Gravity = 0.21f;
	Diffusion = 0.0f;
	HotAir = 0.0f;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 18;
	Weight = 78;
	HeatConduct = 19;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CAPH");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1940.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniInorganicElementUpdate;
}
