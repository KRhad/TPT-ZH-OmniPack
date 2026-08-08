#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_ADIP()
{
	Identifier = "OMNI_PT_ADIP";
	Name = "ADIP";
	OmniConfigureOrganicElement(*this, PT_ADIP);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ADIP");
}
