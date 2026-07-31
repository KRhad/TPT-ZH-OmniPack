#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_F()
{
	Identifier = "OMNI_PT_F";
	Name = "F";
	Colour = 0xDFF56A_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.8f;
	AirDrag = 0.0f;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 2.2f;
	HotAir = 0.0012f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 4;
	HeatConduct = 80;
	HeatCapacity = 0.82f;
	DefaultProperties.temp = 293.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_F");
	Properties = TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &OmniHalogenUpdate;
	Graphics = &OmniHalogenGraphics;
	Create = &OmniHalogenCreate;
}
