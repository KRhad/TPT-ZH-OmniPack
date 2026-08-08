#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_HMET()
{
	Identifier = "OMNI_PT_HMET";
	Name = "HMET";
	OmniConfigureEnvironmentElement(*this, PT_HMET);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_DEADLY | PROP_CONDUCTS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_HMET");
}
