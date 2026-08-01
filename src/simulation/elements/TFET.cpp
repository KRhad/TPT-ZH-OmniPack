#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_TFET()
{
	Identifier = "OMNI_PT_TFET";
	Name = "C2F4";
	OmniConfigureOrganicElement(*this, PT_TFET);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_NEUTPASS | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_TFET");
}
