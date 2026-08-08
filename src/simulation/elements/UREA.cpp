#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_UREA()
{
	Identifier = "OMNI_PT_UREA";
	Name = "UREA";
	OmniConfigureOrganicElement(*this, PT_UREA);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_UREA");
}
