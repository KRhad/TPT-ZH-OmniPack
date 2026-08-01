#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_KPER()
{
	Identifier = "OMNI_PT_KPER";
	Name = "KPER";
	Colour = 0x7A278E_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.45f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.81f;
	Collision = -0.1f;
	Gravity = 0.20f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 12;
	Weight = 78;
	HeatConduct = 24;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_KPER");
	Properties = TYPE_PART | PROP_DEADLY | PROP_NEUTPASS;
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
