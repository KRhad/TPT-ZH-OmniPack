#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_GRAN()
{
	Identifier = "OMNI_PT_GRAN";
	Name = "GRAN";
	OmniConfigureElectronicsElement(*this, PT_GRAN);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_GRAN");
}
