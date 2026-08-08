#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_STYR()
{
	Identifier = "OMNI_PT_STYR";
	Name = "STYR";
	OmniConfigureOrganicElement(*this, PT_STYR);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_STYR");
}
