#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniElectronics.h"

void Element::Element_AERG()
{
	Identifier = "OMNI_PT_AERG";
	Name = "AERG";
	OmniConfigureElectronicsElement(*this, PT_AERG);
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_PHOTPASS | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AERG");
}
