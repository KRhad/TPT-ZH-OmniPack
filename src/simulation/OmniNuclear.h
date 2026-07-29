#pragma once

#include "ElementDefs.h"

// Advanced nuclear updates only inspect a 3x3 neighbourhood and do not alter
// official URAN, PLUT, NEUT or DEUT state machines.
int OmniNuclearElementUpdate(UPDATE_FUNC_ARGS);
int OmniNuclearSparkUpdate(UPDATE_FUNC_ARGS);
