#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_FATS()
{
	Identifier = "OMNI_PT_FATS";
	Name = "FATS";
	OmniConfigureOrganicElement(*this, PT_FATS);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FATS");
}
