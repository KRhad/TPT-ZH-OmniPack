#include "OmniContentPresentation.h"

#include "gui/game/tool/Tool.h"
#include "simulation/ElementDefs.h"
#include "simulation/MenuSection.h"

#include <algorithm>

OmniContentPresentationRecord const *FindOmniContentPresentation(int stableId)
{
	auto records = GetOmniContentPresentationData();
	auto found = std::lower_bound(
		records.begin(), records.end(), stableId,
		[](auto const &record, int value) { return record.stableId < value; });
	return found != records.end() && found->stableId == stableId ? &*found : nullptr;
}

std::optional<int> GetOmniStandardMenuSection(Tool const &tool)
{
	if (!tool.MenuVisible)
		return std::nullopt;
	if (!tool.IsElement)
		return tool.MenuSection;

	auto const *record = FindOmniContentPresentation(TYP(tool.ToolID));
	if (!record || record->identifier != std::string_view(tool.Identifier.data(), tool.Identifier.size()))
		return tool.MenuSection;

	switch (record->mainMenuPolicy)
	{
	case OmniMainMenuPolicy::PeriodicOnly:
	case OmniMainMenuPolicy::Hidden:
		return std::nullopt;
	case OmniMainMenuPolicy::OrganicMenu:
		return SC_OMNI_ORGANIC;
	case OmniMainMenuPolicy::AlloyMenu:
		return SC_OMNI_ALLOY;
	case OmniMainMenuPolicy::OfficialDefault:
	case OmniMainMenuPolicy::ExistingDedicatedMenu:
	case OmniMainMenuPolicy::PreserveExisting:
		return tool.MenuSection;
	}
	return tool.MenuSection;
}
