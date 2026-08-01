#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_ERES()
{
	Identifier = "OMNI_PT_ERES";
	Name = "ERES";
	OmniConfigureOrganicElement(*this, PT_ERES);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ERES");
}
