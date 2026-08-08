#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_ALCR()
{
	Identifier = "OMNI_PT_ALCR";
	Name = "ALCR";
	Colour = 0xE3E5E6_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Advection = 0.0f;
	AirDrag = 0.0f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 15;
	Weight = 92;
	HeatConduct = 45;
	HeatCapacity = 1.18f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ALCR");
	Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_HOT_GLOW;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 220.0f;
	HighPressureTransition = PT_STNE;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2600.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
