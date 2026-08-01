#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_ACET()
{
	Identifier = "OMNI_PT_ACET";
	Name = "ACET";
	OmniConfigureOrganicElement(*this, PT_ACET);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ACET");
}
