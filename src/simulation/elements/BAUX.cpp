#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_BAUX()
{
	Identifier = "OMNI_PT_BAUX";
	Name = "BAUX";
	Colour = 0xA85F3D_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.36f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.87f;
	Collision = -0.1f;
	Gravity = 0.25f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 20;
	Weight = 90;
	HeatConduct = 24;
	HeatCapacity = 0.96f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BAUX");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1870.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
