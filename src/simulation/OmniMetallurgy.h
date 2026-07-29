#pragma once

#include "ElementDefs.h"

// All routines in this file are deliberately local-neighbour updates.  They
// must never scan the global particle array from an element update.
int OmniMetallurgyMetalUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyScrapUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyLavaUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyWoodUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyCoalUpdate(UPDATE_FUNC_ARGS);
