#pragma once

#include "ElementDefs.h"

// Periodic-table family logic is deliberately local and budgeted. Individual
// elements keep their own physical properties while related reactions share a
// single implementation.
int OmniNobleGasUpdate(UPDATE_FUNC_ARGS);
int OmniNobleGasGraphics(GRAPHICS_FUNC_ARGS);
void OmniNobleGasCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniAlkaliMetalUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenAlkaliUpdate(UPDATE_FUNC_ARGS);
void OmniAlkaliMetalCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniAlkalineEarthMetalUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenAlkalineEarthUpdate(UPDATE_FUNC_ARGS);
void OmniAlkalineEarthMetalCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniBoronGroupUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenBoronGroupUpdate(UPDATE_FUNC_ARGS);
void OmniBoronGroupCreate(ELEMENT_CREATE_FUNC_ARGS);
