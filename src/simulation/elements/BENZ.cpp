#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_BENZ()
{
	Identifier = "OMNI_PT_BENZ";
	Name = "BENZ";
	OmniConfigureOrganicElement(*this, PT_BENZ);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BENZ");
}
