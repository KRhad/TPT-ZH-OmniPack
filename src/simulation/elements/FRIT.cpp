#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_FRIT()
{
	Identifier = "OMNI_PT_FRIT";
	Name = "FRIT";
	OmniConfigureElectronicsElement(*this, PT_FRIT);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTABSORB | PROP_LIFE_DEC;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_FRIT");
}
