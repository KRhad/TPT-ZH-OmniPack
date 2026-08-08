#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_ACTA()
{
	Identifier = "OMNI_PT_ACTA";
	Name = "ACTA";
	OmniConfigureOrganicElement(*this, PT_ACTA);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ACTA");
}
