#pragma once

#include "ElementClasses.h"

#include <array>
#include <span>

struct OmniAlloyConstituent
{
	int elementType;
	double recipeFraction;
};

struct OmniAlloyCompositionDefinition
{
	int alloyType;
	const char *modelId;
	const char *status;
	std::array<OmniAlloyConstituent, 3> constituents;
	size_t constituentCount;
};

inline constexpr std::array<OmniAlloyCompositionDefinition, 5> OmniEngineeringAlloyCompositions{ {
	{ PT_TIAL, "recipe.tial.4ti_1al_1v", "game_recipe_fraction",
		{ OmniAlloyConstituent{ PT_TTAN, 4.0 / 6.0 }, OmniAlloyConstituent{ PT_ALUM, 1.0 / 6.0 },
			OmniAlloyConstituent{ PT_V, 1.0 / 6.0 } }, 3 },
	{ PT_NSAL, "recipe.nsal.4ni_1cr_1co", "game_recipe_fraction",
		{ OmniAlloyConstituent{ PT_NICL, 4.0 / 6.0 }, OmniAlloyConstituent{ PT_CHRM, 1.0 / 6.0 },
			OmniAlloyConstituent{ PT_COBT, 1.0 / 6.0 } }, 3 },
	{ PT_WALY, "recipe.waly.4w_1ni_1fe", "game_recipe_fraction",
		{ OmniAlloyConstituent{ PT_TUNG, 4.0 / 6.0 }, OmniAlloyConstituent{ PT_NICL, 1.0 / 6.0 },
			OmniAlloyConstituent{ PT_IRON, 1.0 / 6.0 } }, 3 },
	{ PT_ZRAL, "recipe.zral.4zr_1sn", "game_recipe_fraction",
		{ OmniAlloyConstituent{ PT_ZR, 4.0 / 5.0 }, OmniAlloyConstituent{ PT_TIN, 1.0 / 5.0 },
			OmniAlloyConstituent{ PT_NONE, 0.0 } }, 2 },
	{ PT_NITI, "recipe.niti.1ni_1ti", "game_recipe_fraction",
		{ OmniAlloyConstituent{ PT_NICL, 0.5 }, OmniAlloyConstituent{ PT_TTAN, 0.5 },
			OmniAlloyConstituent{ PT_NONE, 0.0 } }, 2 },
} };

inline const OmniAlloyCompositionDefinition *OmniAlloyCompositionForType(int type)
{
	for (const auto &definition : OmniEngineeringAlloyCompositions)
		if (definition.alloyType == type)
			return &definition;
	return nullptr;
}
