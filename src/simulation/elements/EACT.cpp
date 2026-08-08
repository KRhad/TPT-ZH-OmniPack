#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_EACT()
{
	Identifier = "OMNI_PT_EACT";
	Name = "EACT";
	OmniConfigureOrganicElement(*this, PT_EACT);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_EACT");
}
