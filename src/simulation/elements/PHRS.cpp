#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_PHRS()
{
	Identifier = "OMNI_PT_PHRS";
	Name = "PHRS";
	OmniConfigureElectronicsElement(*this, PT_PHRS);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_PHRS");
}
