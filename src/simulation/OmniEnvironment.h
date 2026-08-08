#pragma once

#include "ElementDefs.h"

class Element;

// Ecology-expansion reactions share the Biology module's 1024 successful
// events per frame and inspect only the current particle's 3x3 neighbourhood.
bool OmniIsEnvironmentElement(int type);
void OmniConfigureEnvironmentElement(Element &element, int type);
void OmniEnvironmentCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniEnvironmentElementUpdate(UPDATE_FUNC_ARGS);
int OmniEnvironmentGraphics(GRAPHICS_FUNC_ARGS);
