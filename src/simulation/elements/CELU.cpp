#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_CELU()
{
	Identifier = "OMNI_PT_CELU";
	Name = "CELU";
	OmniConfigureOrganicElement(*this, PT_CELU);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CELU");
}
