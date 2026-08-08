#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_EPXY()
{
	Identifier = "OMNI_PT_EPXY";
	Name = "EPXY";
	OmniConfigureOrganicElement(*this, PT_EPXY);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_EPXY");
}
