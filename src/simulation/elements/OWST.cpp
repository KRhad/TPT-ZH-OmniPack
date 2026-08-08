#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_OWST()
{
	Identifier = "OMNI_PT_OWST";
	Name = "OWST";
	OmniConfigureEnvironmentElement(*this, PT_OWST);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_OWST");
}
