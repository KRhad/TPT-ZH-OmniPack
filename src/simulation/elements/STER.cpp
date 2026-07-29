#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniBiology.h"

void Element::Element_STER()
{
	Identifier = "OMNI_PT_STER";
	Name = "STER";
	Colour = 0x7ED7E8_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Advection = 0.42f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.82f;
	Collision = -0.1f;
	Gravity = 0.16f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 1;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 10;
	Weight = 62;
	HeatConduct = 44;
	HeatCapacity = 1.12f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_STER");
	Properties = TYPE_PART | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 700.0f;
	HighTemperatureTransition = PT_FIRE;
	Update = &OmniBiologyElementUpdate;
}
