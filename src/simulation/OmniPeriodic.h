#pragma once

#include "ElementDefs.h"

// Periodic-table family logic is deliberately local and budgeted. Individual
// elements keep their own physical properties while related reactions share a
// single implementation.
int OmniNobleGasUpdate(UPDATE_FUNC_ARGS);
int OmniNobleGasGraphics(GRAPHICS_FUNC_ARGS);
void OmniNobleGasCreate(ELEMENT_CREATE_FUNC_ARGS);
