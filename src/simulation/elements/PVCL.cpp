#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniOrganics.h"

void Element::Element_PVCL()
{
	Identifier = "OMNI_PT_PVCL";
	Name = "PVC";
	OmniConfigureOrganicElement(*this, PT_PVCL);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_DEADLY;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PVCL");
}
