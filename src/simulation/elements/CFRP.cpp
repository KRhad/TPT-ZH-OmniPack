#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_CFRP()
{
	Identifier = "OMNI_PT_CFRP";
	Name = "CFRP";
	OmniConfigureElectronicsElement(*this, PT_CFRP);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_CFRP");
}
