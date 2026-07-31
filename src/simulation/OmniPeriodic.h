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
int OmniCarbonGroupUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenCarbonGroupUpdate(UPDATE_FUNC_ARGS);
int OmniCarbonGroupGraphics(GRAPHICS_FUNC_ARGS);
void OmniCarbonGroupCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniNitrogenGroupUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenNitrogenGroupUpdate(UPDATE_FUNC_ARGS);
int OmniNitrogenGroupGraphics(GRAPHICS_FUNC_ARGS);
void OmniNitrogenGroupCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniOxygenGroupUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenOxygenGroupUpdate(UPDATE_FUNC_ARGS);
int OmniOxygenGroupGraphics(GRAPHICS_FUNC_ARGS);
void OmniOxygenGroupCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniHalogenUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenHalogenUpdate(UPDATE_FUNC_ARGS);
int OmniHalogenGraphics(GRAPHICS_FUNC_ARGS);
void OmniHalogenCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniFirstTransitionUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenFirstTransitionUpdate(UPDATE_FUNC_ARGS);
int OmniFirstTransitionGraphics(GRAPHICS_FUNC_ARGS);
int OmniSecondTransitionUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenSecondTransitionUpdate(UPDATE_FUNC_ARGS);
int OmniSecondTransitionGraphics(GRAPHICS_FUNC_ARGS);
void OmniSecondTransitionCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniThirdTransitionUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenThirdTransitionUpdate(UPDATE_FUNC_ARGS);
int OmniThirdTransitionGraphics(GRAPHICS_FUNC_ARGS);
void OmniThirdTransitionCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniLanthanideUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenLanthanideUpdate(UPDATE_FUNC_ARGS);
int OmniLanthanideGraphics(GRAPHICS_FUNC_ARGS);
void OmniLanthanideCreate(ELEMENT_CREATE_FUNC_ARGS);
int OmniActinideUpdate(UPDATE_FUNC_ARGS);
int OmniMoltenActinideUpdate(UPDATE_FUNC_ARGS);
int OmniActinideGraphics(GRAPHICS_FUNC_ARGS);
void OmniActinideCreate(ELEMENT_CREATE_FUNC_ARGS);
