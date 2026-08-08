#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_AMAT()
{
	Identifier = "OMNI_PT_AMAT";
	Name = "AMAT";
	OmniConfigureEnvironmentElement(*this, PT_AMAT);
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;
	Properties = TYPE_SOLID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_AMAT");
}
