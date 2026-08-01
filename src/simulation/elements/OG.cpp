#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_OG()
{
	Identifier = "OMNI_PT_OG";
	Name = "OGAN";
	Colour = 0xD95CA8_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.45f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.980f;
	Loss = 0.30f;
	Collision = -0.02f;
	Gravity = 0.08f;
	Diffusion = 0.40f;
	HotAir = 0.0004f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x03F03F00;
	Weight = 30;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 8;
	HeatCapacity = 1.25f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_OG");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC |
		PROP_RADIOACTIVE | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 4300.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
