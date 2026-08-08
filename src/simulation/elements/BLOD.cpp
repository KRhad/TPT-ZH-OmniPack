#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

// Blood transport and clotting concepts were reviewed in GPL-3.0 Biological
// Mod and Ultimata sources; this bounded 3x3 implementation is independent.
void Element::Element_BLOD()
{
	Identifier = "OMNI_PT_BLOD";
	Name = "BLOD";
	OmniConfigureEnvironmentElement(*this, PT_BLOD);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;
	Properties = TYPE_LIQUID | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_BLOD");
}
