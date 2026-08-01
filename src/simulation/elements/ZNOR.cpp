#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_ZNOR()
{
	Identifier = "OMNI_PT_ZNOR";
	Name = "ZNOR";
	Colour = 0xB5A46B_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.38f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.87f;
	Collision = -0.1f;
	Gravity = 0.26f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 22;
	Weight = 104;
	HeatConduct = 26;
	HeatCapacity = 0.84f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ZNOR");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1500.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
