#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_DIAM()
{
	Identifier = "OMNI_PT_DIAM";
	Name = "DIAM";
	OmniConfigureOrganicElement(*this, PT_DIAM);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_DIAM");
}
