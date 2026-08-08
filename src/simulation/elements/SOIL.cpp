#include "simulation/ElementCommon.h"
#include "common/Localization.h"
#include "simulation/OmniEnvironment.h"

// Soil-as-a-water-buffer concept reviewed from GPL-3.0 nucular and Ultimata
// sources; all state, reactions, limits, and graphics below are reimplemented.
void Element::Element_SOIL()
{
	Identifier = "OMNI_PT_SOIL";
	Name = "SOIL";
	OmniConfigureEnvironmentElement(*this, PT_SOIL);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;
	Properties = TYPE_PART | PROP_NEUTPASS;
	Description = Localization::Ref().Tr("sim.elem.OMNI_PT_SOIL");
}
