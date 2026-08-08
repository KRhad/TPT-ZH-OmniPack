#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

void Element::Element_RCON()
{
	Identifier = "OMNI_PT_RCON";
	Name = "RCON";
	OmniConfigureEnvironmentElement(*this, PT_RCON);
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;
	Properties = TYPE_PART | PROP_DEADLY | PROP_RADIOACTIVE | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_RCON");
}
