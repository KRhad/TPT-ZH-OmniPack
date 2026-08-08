#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_GAAS()
{
	Identifier = "OMNI_PT_GAAS";
	Name = "GAAS";
	OmniConfigureElectronicsElement(*this, PT_GAAS);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_GAAS");
}
