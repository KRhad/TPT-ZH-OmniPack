#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniMaterials.h"

void Element::Element_GYPS()
{
	Identifier = "OMNI_PT_GYPS";
	Name = "GYPS";
	Colour = 0xD8CEC0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.38f;
	AirDrag = 0.03f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.88f;
	Collision = -0.1f;
	Gravity = 0.24f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 12;
	Weight = 80;
	HeatConduct = 18;
	HeatCapacity = 1.25f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_GYPS");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1720.0f;
	HighTemperatureTransition = PT_LAVA;
	Update = &OmniMaterialsElementUpdate;
}
