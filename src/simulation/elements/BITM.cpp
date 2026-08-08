#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_BITM()
{
	Identifier = "OMNI_PT_BITM";
	Name = "BITM";
	OmniConfigureOrganicElement(*this, PT_BITM);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BITM");
}
