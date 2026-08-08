#pragma once

#include "ElementDefs.h"

// Biology updates are limited to the current particle's 3x3 neighbourhood.
// They intentionally leave the official PLNT, VIRS, WATR and LIFE state
// machines untouched.
bool OmniBiologyModuleEnabled(Simulation *sim);
bool OmniConsumeBiologyEvent(Simulation *sim);
int OmniBiologyElementUpdate(UPDATE_FUNC_ARGS);
