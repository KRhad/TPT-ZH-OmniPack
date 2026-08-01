#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_PBOR()
{
	Identifier = "OMNI_PT_PBOR";
	Name = "PBOR";
	Colour = 0x5F6266_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.34f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.84f;
	Collision = -0.1f;
	Gravity = 0.30f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 18;
	Weight = 132;
	HeatConduct = 30;
	HeatCapacity = 0.76f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PBOR");
	Properties = TYPE_PART;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1350.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
