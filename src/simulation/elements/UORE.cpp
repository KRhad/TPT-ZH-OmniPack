#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_UORE()
{
	Identifier = "OMNI_PT_UORE";
	Name = "UORE";
	Colour = 0x536B42_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Advection = 0.32f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.83f;
	Collision = -0.1f;
	Gravity = 0.31f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 26;
	Weight = 142;
	HeatConduct = 22;
	HeatCapacity = 0.82f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_UORE");
	Properties = TYPE_PART | PROP_RADIOACTIVE | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1800.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
