#include "simulation/ElementCommon.h"
#include "simulation/OmniChemistry.h"
#include "common/Localization.h"

void Element::Element_NITA()
{
	Identifier = "OMNI_PT_NITA";
	Name = "NITA";
	Colour = 0xFFF2A8_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Advection = 0.62f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.11f;
	Diffusion = 0.0f;
	HotAir = 0.0f * CFDS;
	Falldown = 2;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 4;
	Weight = 50;
	HeatConduct = 34;
	HeatCapacity = 1.80f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NITA");
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 410.0f;
	HighTemperatureTransition = PT_CAUS;
	Update = &OmniInorganicElementUpdate;
}
