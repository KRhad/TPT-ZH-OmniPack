#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_MPLS()
{
	Identifier = "OMNI_PT_MPLS";
	Name = "MPLS";
	OmniConfigureEnvironmentElement(*this, PT_MPLS);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_MPLS");
}
