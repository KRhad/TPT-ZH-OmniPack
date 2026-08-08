#pragma once

#include "ElementDefs.h"

// Shared soft rendering for OmniPack gases. Specialised gas renderers call
// this first, then add their own discharge or radioactive glow.
int OmniGasGraphics(GRAPHICS_FUNC_ARGS);
