#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_ETHE()
{
	Identifier = "OMNI_PT_ETHE";
	Name = "C2H4";
	OmniConfigureOrganicElement(*this, PT_ETHE);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ETHE");
}
