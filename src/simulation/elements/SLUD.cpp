#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_SLUD()
{
	Identifier = "OMNI_PT_SLUD";
	Name = "SLUD";
	OmniConfigureEnvironmentElement(*this, PT_SLUD);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SLUD");
}
