#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMetallurgy.h"

void Element::Element_MSCR()
{
	Identifier = "OMNI_PT_MSCR";
	Name = "MSCR";
	Colour = 0x55585B_rgb;
	// Compatibility alias only. New placement and search resolve to the
	// canonical official BRMT implementation.
	MenuVisible = 0;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.15f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.85f;
	Collision = -0.1f;
	Gravity = 0.28f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 8;
	Weight = 95;
	HeatConduct = 120;
	HeatCapacity = 1.00f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_MSCR");
	Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW;
	CarriesTypeIn = 1U << FIELD_CTYPE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &OmniMetallurgyLegacyScrapAliasUpdate;
}
