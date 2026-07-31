#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniChemistry.h"
#include "simulation/OmniPeriodic.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_CHLR()
{
	Identifier = "OMNI_PT_CHLR";
	Name = "CHLR";
	Colour = 0x95C84B_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Advection = 1.2f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.10f;
	Diffusion = 0.55f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;
	Weight = 24;
	HeatConduct = 48;
	HeatCapacity = 1.10f;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CHLR");
	Properties = TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS;
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (OmniChemistryElementUpdate(UPDATE_FUNC_SUBCALL_ARGS))
	{
		return 1;
	}
	return OmniHalogenUpdate(UPDATE_FUNC_SUBCALL_ARGS);
}
