#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_PSTY()
{
	Identifier = "OMNI_PT_PSTY";
	Name = "PSTY";
	OmniConfigureOrganicElement(*this, PT_PSTY);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PSTY");
}
