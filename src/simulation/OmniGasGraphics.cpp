#include "OmniGasGraphics.h"

#include "ElementCommon.h"

#include <algorithm>
#include <cstdlib>

int OmniGasGraphics(GRAPHICS_FUNC_ARGS)
{
	int warmth = std::clamp(int((cpart->temp - 293.15f) / 18.0f), -18, 42);
	if (warmth > 0)
	{
		*colr = std::min(*colr + warmth, 255);
		*colg = std::min(*colg + warmth / 3, 255);
		*colb = std::max(*colb - warmth / 4, 0);
	}
	else if (warmth < 0)
	{
		*colb = std::min(*colb - warmth, 255);
	}

	// Match the renderer used by official TYPE_GAS elements: the particle is
	// represented by a soft fire-layer cloud instead of a solid centre pixel.
	*pixel_mode &= ~PMODE;
	*pixel_mode |= FIRE_BLEND | DECO_FIRE;
	*firer = *colr / 2;
	*fireg = *colg / 2;
	*fireb = *colb / 2;
	*firea = 125;
	return 0;
}
