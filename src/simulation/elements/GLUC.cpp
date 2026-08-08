#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_GLUC()
{
	Identifier = "OMNI_PT_GLUC";
	Name = "GLUC";
	OmniConfigureOrganicElement(*this, PT_GLUC);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_GLUC");
}
