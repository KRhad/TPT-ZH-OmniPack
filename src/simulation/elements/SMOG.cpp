#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_SMOG()
{
	Identifier = "OMNI_PT_SMOG";
	Name = "SMOG";
	OmniConfigureEnvironmentElement(*this, PT_SMOG);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_DEADLY | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SMOG");
}
