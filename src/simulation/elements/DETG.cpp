#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_DETG()
{
	Identifier = "OMNI_PT_DETG";
	Name = "DETG";
	OmniConfigureEnvironmentElement(*this, PT_DETG);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_DETG");
}
