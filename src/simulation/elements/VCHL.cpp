#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_VCHL()
{
	Identifier = "OMNI_PT_VCHL";
	Name = "VCM";
	OmniConfigureOrganicElement(*this, PT_VCHL);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;
	Properties = TYPE_GAS | PROP_NEUTPASS | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_VCHL");
}
