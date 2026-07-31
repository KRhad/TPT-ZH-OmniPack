#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_NE()
{
	Identifier = "OMNI_PT_NE";
	Name = "NE";
	Colour = 0xFF6A4D_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.8f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.992f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = 0.00f;
	Diffusion = 2.60f;
	HotAir = 0.0001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x0007C000;
	Weight = 2;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 90;
	HeatCapacity = 0.55f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_NE");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 7500.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
