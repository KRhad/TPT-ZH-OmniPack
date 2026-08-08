#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_ARAN()
{
	Identifier = "OMNI_PT_ARAN";
	Name = "ARAN";
	OmniConfigureEnvironmentElement(*this, PT_ARAN);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_DEADLY | PROP_CONDUCTS | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_ARAN");
}
