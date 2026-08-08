#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_QGLS()
{
	Identifier = "OMNI_PT_QGLS";
	Name = "QGLS";
	Colour = 0xD2E8EF_rgb;
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
	Hardness = 4;
	Weight = 96;
	HeatConduct = 170;
	HeatCapacity = 1.05f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_QGLS");
	Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_PHOTPASS | PROP_HOT_GLOW;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2250.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
