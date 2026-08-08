#pragma once

#include "ElementDefs.h"

// Mineral, ceramic, glass, and construction-material reactions remain local
// to the current 3x3 neighbourhood and share a bounded per-frame event budget.
int OmniMaterialsElementUpdate(UPDATE_FUNC_ARGS);
int OmniMaterialsLavaUpdate(UPDATE_FUNC_ARGS);
int OmniMaterialsSparkUpdate(UPDATE_FUNC_ARGS);

// The fork's two special glasses participate in the same refraction boundary
// rules as upstream GLAS/BGLA without replacing either official element.
bool IsGlassMaterialType(int type);
