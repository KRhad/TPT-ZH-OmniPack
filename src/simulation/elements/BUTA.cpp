#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_BUTA()
{
	Identifier = "OMNI_PT_BUTA";
	Name = "BUTA";
	OmniConfigureOrganicElement(*this, PT_BUTA);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BUTA");
}
