#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_KR()
{
	Identifier = "OMNI_PT_KR";
	Name = "KRYP";
	Colour = 0xDCEBFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.988f;
	Loss = 0.30f;
	Collision = -0.08f;
	Gravity = 0.02f;
	Diffusion = 1.20f;
	HotAir = 0.0001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x000FF000;
	Weight = 8;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 30;
	HeatCapacity = 0.85f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_KR");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 5900.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
