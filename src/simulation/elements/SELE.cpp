#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_SELE()
{
	Identifier = "OMNI_PT_SELE";
	Name = "SELE";
	OmniConfigureElectronicsElement(*this, PT_SELE);
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SELE");
}
