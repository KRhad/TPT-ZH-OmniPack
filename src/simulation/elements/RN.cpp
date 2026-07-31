#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_RN()
{
	Identifier = "OMNI_PT_RN";
	Name = "RN";
	Colour = 0xB8D96B_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.70f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.984f;
	Loss = 0.30f;
	Collision = -0.04f;
	Gravity = 0.06f;
	Diffusion = 0.65f;
	HotAir = 0.0003f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x0003FFF0;
	Weight = 22;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 10;
	HeatCapacity = 1.15f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_RN");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC |
		PROP_RADIOACTIVE | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 4800.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
