#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_PEST()
{
	Identifier = "OMNI_PT_PEST";
	Name = "PEST";
	OmniConfigureEnvironmentElement(*this, PT_PEST);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PEST");
}
