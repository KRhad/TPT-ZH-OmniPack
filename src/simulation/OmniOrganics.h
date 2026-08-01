#pragma once

#include "ElementDefs.h"

class Element;

// Organic reactions share the advanced-chemistry 1536/frame event budget and
// inspect only the current particle's 3x3 neighbourhood.
void OmniConfigureOrganicElement(Element &element, int type);
int OmniOrganicElementUpdate(UPDATE_FUNC_ARGS);
