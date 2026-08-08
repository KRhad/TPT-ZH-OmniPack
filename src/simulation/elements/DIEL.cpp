#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_DIEL()
{
	Identifier = "OMNI_PT_DIEL";
	Name = "DIEL";
	OmniConfigureElectronicsElement(*this, PT_DIEL);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_LIFE_DEC;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_DIEL");
}
