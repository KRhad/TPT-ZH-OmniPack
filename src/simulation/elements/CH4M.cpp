#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_CH4M()
{
	Identifier = "OMNI_PT_CH4M";
	Name = "CH4";
	OmniConfigureOrganicElement(*this, PT_CH4M);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CH4M");
}
