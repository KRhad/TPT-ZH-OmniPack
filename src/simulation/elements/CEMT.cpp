#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_CEMT()
{
	Identifier = "OMNI_PT_CEMT";
	Name = "CEMT";
	Colour = 0xAAA79F_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.40f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.90f;
	Collision = -0.1f;
	Gravity = 0.27f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 10;
	Weight = 72;
	HeatConduct = 35;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CEMT");
	Properties = TYPE_PART | PROP_LIFE_DEC;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1100.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
