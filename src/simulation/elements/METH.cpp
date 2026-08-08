#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_METH()
{
	Identifier = "OMNI_PT_METH";
	Name = "MEOH";
	OmniConfigureOrganicElement(*this, PT_METH);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_METH");
}
