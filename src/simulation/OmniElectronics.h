#pragma once

#include "ElementDefs.h"

class Element;

// Electronic-material reactions use only the current particle's 3x3
// neighbourhood and share a strict 1024-successful-events-per-frame budget.
bool OmniElectronicsModuleEnabled(Simulation *sim);
bool OmniIsElectronicsElement(int type);
void OmniConfigureElectronicsElement(Element &element, int type);
int OmniElectronicsCatalystUpdate(UPDATE_FUNC_ARGS);
int OmniElectronicsElementUpdate(UPDATE_FUNC_ARGS);
int OmniElectronicsSparkUpdate(UPDATE_FUNC_ARGS);
int OmniElectronicsGraphics(GRAPHICS_FUNC_ARGS);
