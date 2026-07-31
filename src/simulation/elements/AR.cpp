#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_AR()
{
	Identifier = "OMNI_PT_AR";
	Name = "AR";
	Colour = 0xB98EFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.3f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.990f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = 0.01f;
	Diffusion = 1.80f;
	HotAir = 0.0001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x001F8000;
	Weight = 4;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 45;
	HeatCapacity = 0.70f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AR");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 6500.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
