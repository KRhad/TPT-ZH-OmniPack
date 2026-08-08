#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_CACO()
{
	Identifier = "OMNI_PT_CACO";
	Name = "CACO";
	Colour = 0xDDD8C5_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.40f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.85f;
	Collision = -0.1f;
	Gravity = 0.22f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 25;
	Weight = 78;
	HeatConduct = 24;
	HeatCapacity = 0.95f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CACO");
	Properties = TYPE_PART | PROP_NEUTPASS;
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
