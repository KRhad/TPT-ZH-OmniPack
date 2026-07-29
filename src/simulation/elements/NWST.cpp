#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniNuclear.h"

void Element::Element_NWST()
{
	Identifier = "OMNI_PT_NWST";
	Name = "NWST";
	Colour = 0x826E50_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Advection = 0.2f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 12;
	Weight = 84;
	HeatConduct = 150;
	HeatCapacity = 1.50f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NWST");
	Properties = TYPE_PART | PROP_RADIOACTIVE | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2400.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniNuclearElementUpdate;
}
