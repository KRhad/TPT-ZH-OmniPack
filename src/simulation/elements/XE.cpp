#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_XE()
{
	Identifier = "OMNI_PT_XE";
	Name = "XE";
	Colour = 0x668CFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.85f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.986f;
	Loss = 0.30f;
	Collision = -0.06f;
	Gravity = 0.04f;
	Diffusion = 0.85f;
	HotAir = 0.0002f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x03FC0000;
	Weight = 13;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 18;
	HeatCapacity = 1.00f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_XE");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 5200.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
