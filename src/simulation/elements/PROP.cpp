#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_PROP()
{
	Identifier = "OMNI_PT_PROP";
	Name = "PROP";
	OmniConfigureOrganicElement(*this, PT_PROP);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PROP");
}
