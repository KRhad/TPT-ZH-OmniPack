#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_GLYC()
{
	Identifier = "OMNI_PT_GLYC";
	Name = "GLYC";
	OmniConfigureOrganicElement(*this, PT_GLYC);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_GLYC");
}
