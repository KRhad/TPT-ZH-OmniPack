#pragma once

#include "ElementDefs.h"

struct Particle;

// Representative isotope updates share the advanced-nuclear 512/frame budget,
// remain local to a 3x3 neighbourhood, and never replace official DEUT/URAN/
// PLUT or the pure-element periodic-table mappings.
int OmniIsotopeElementUpdate(UPDATE_FUNC_ARGS);
int OmniIsotopeLavaUpdate(UPDATE_FUNC_ARGS);
int OmniIsotopeSourceUpdate(UPDATE_FUNC_ARGS);
int OmniIsotopeGraphics(GRAPHICS_FUNC_ARGS);
void OmniIsotopeCreate(ELEMENT_CREATE_FUNC_ARGS);
bool OmniIsotopeOwnsMoltenTransition(Particle const &particle);
