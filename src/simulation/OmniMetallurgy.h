#pragma once

#include "ElementDefs.h"

// Recoverable OmniPack scrap is stored in the official BRMT element.  The
// marker keeps the enhanced source-metal recovery path separate from ordinary
// upstream BRMT behavior while preserving ctype through OPS saves.
constexpr int OmniRecoverableScrapMarker = 0x4F4D5343; // "OMSC"

bool IsOmniRecoverableScrap(Particle const &particle);
bool IsOmniRecoverableMetalType(int type);
void MarkOmniRecoverableScrap(Particle &particle, int sourceType);

// All routines in this file are deliberately local-neighbour updates.  They
// must never scan the global particle array from an element update.
int OmniMetallurgyMetalUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyScrapUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyLegacyScrapAliasUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyLavaUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgySparkUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyWoodUpdate(UPDATE_FUNC_ARGS);
int OmniMetallurgyCoalUpdate(UPDATE_FUNC_ARGS);
