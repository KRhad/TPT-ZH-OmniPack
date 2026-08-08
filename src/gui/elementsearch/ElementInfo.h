#pragma once

#include "common/String.h"

class Tool;

// Adds the short player-facing gesture hint used beside element descriptions.
String ElementDescriptionWithLongPressHint(String description);

// Opens the player-facing, scrollable element reference used by menu buttons,
// search results and material buttons in periodic-table details.
void OpenElementInfo(Tool const *tool);
