#include "simulation/ElementCommon.h"
#include "simulation/OmniPeriodic.h"
#include "common/Localization.h"

void Element::Element_HE()
{
	Identifier = "OMNI_PT_HE";
	Name = "HELI";
	Colour = 0xD9FFFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.4f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.995f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = -0.01f;
	Diffusion = 4.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	PhotonReflectWavelengths = 0x03F00000;
	Weight = 1;
	DefaultProperties.temp = 293.15f;
	HeatConduct = 255;
	HeatCapacity = 0.35f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_HE");
	Properties = TYPE_GAS | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 9000.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &OmniNobleGasUpdate;
	Graphics = &OmniNobleGasGraphics;
	Create = &OmniNobleGasCreate;
}
