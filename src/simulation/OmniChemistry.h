#pragma once

#include "ElementDefs.h"

// Chemistry updates are confined to the current particle's 3x3 neighbourhood.
// They deliberately avoid changing the state machines of official GAS/OIL/WAX.
bool OmniChemistryModuleEnabled(Simulation *sim);
bool OmniConsumeChemistryEvent(Simulation *sim);
int OmniChemistryElementUpdate(UPDATE_FUNC_ARGS);
int OmniInorganicElementUpdate(UPDATE_FUNC_ARGS);
int OmniChemistryYeastUpdate(UPDATE_FUNC_ARGS);
int OmniChemistrySparkUpdate(UPDATE_FUNC_ARGS);
