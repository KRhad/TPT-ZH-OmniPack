#include "ElementInfo.h"

#include "ElementCatalog.h"
#include "common/Localization.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/game/tool/Tool.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/ElementDefs.h"
#include "simulation/SimulationData.h"
#include "simulation/TransitionConstants.h"

#include <string_view>
#include <utility>

namespace
{
String CatalogString(std::string_view value)
{
	return ByteString(value.data(), value.size()).FromUtf8();
}

String CatalogValue(char const *kind, std::string_view value)
{
	ByteString key = "encyclopedia.value.";
	key += kind;
	key += ".";
	key.append(value.data(), value.size());
	auto fallback = ByteString(value.data(), value.size());
	return Localization::Ref().Tr(key.c_str(), fallback.c_str());
}

String CatalogCategory(std::string_view category)
{
	static constexpr std::pair<std::string_view, char const *> categoryKeys[] = {
		{ "SC_WALL", "sim.menu.walls" },
		{ "SC_ELEC", "sim.menu.electronics" },
		{ "SC_POWERED", "sim.menu.powered" },
		{ "SC_SENSOR", "sim.menu.sensors" },
		{ "SC_FORCE", "sim.menu.force" },
		{ "SC_EXPLOSIVE", "sim.menu.explosives" },
		{ "SC_GAS", "sim.menu.gases" },
		{ "SC_LIQUID", "sim.menu.liquids" },
		{ "SC_POWDERS", "sim.menu.powders" },
		{ "SC_SOLIDS", "sim.menu.solids" },
		{ "SC_NUCLEAR", "sim.menu.radioactive" },
		{ "SC_SPECIAL", "sim.menu.special" },
		{ "SC_LIFE", "sim.menu.gol" },
		{ "SC_TOOL", "sim.menu.tools" },
		{ "SC_FAVORITES", "sim.menu.favorites" },
		{ "SC_DECO", "sim.menu.deco" },
		{ "SC_OMNI_ORGANIC", "sim.menu.omni_organic" },
		{ "SC_OMNI_ALLOY", "sim.menu.omni_alloy" },
	};
	for (auto const &[value, key] : categoryKeys)
	{
		if (category == value)
			return Localization::Ref().Tr(key);
	}
	return CatalogValue("category", category);
}

String RuntimeCategory(Element const &element)
{
	auto const &sections = SimulationData::Ref().msections;
	if (element.MenuSection >= 0 && element.MenuSection < int(sections.size()))
		return Localization::Ref().Tr(sections[element.MenuSection].name.ToUtf8().c_str());
	return CatalogValue("state", "special");
}

String RuntimeState(Element const &element)
{
	std::string_view state = "special";
	if (element.Properties & TYPE_ENERGY)
		state = "energy";
	else if (element.Properties & TYPE_GAS)
		state = "gas";
	else if (element.Properties & TYPE_LIQUID)
		state = "liquid";
	else if (element.Properties & TYPE_PART)
		state = "powder";
	else if (element.Properties & TYPE_SOLID)
		state = "solid";
	return CatalogValue("state", state);
}

String CatalogContent(
	int language,
	std::string_view english,
	std::string_view chinese)
{
	return language == 1 && !chinese.empty()
		? CatalogString(chinese)
		: CatalogString(english);
}
}

String ElementDescriptionWithLongPressHint(String description)
{
	auto hint = Localization::Ref().Tr("element.long_press_hint");
	if (description.empty())
		return hint;
	return String::Build(std::move(description), " · ", hint);
}

void OpenElementInfo(Tool const *tool)
{
	if (!tool || !tool->IsElement)
		return;
	auto elementId = TYP(tool->ToolID);
	if (elementId <= 0 || elementId >= PT_NUM)
		return;

	auto const &element = SimulationData::Ref().elements[elementId];
	auto const *record = FindElementCatalogByIdentifier(tool->Identifier);
	auto language = GlobalPrefs::Ref().Get("Language", 1);

	String title = tool->Name;
	if (record)
	{
		auto currentName = language == 1 && !record->chineseName.empty()
			? CatalogString(record->chineseName)
			: CatalogString(record->englishName);
		title = language == 1
			? String::Build(currentName, "（", CatalogString(record->displayCode), "）")
			: String::Build(currentName, " (", CatalogString(record->displayCode), ")");
	}

	auto description = element.Description;
	if (record)
	{
		auto catalogDescription = language == 1
			? CatalogString(record->chineseDescription)
			: CatalogString(record->englishDescription);
		if (!catalogDescription.empty())
			description = std::move(catalogDescription);
	}

	StringBuilder details;
	details << Localization::Ref().Tr("encyclopedia.description") << "\n"
		<< description << "\n\n";
	details << Localization::Ref().Tr("encyclopedia.category") << ": "
		<< (record ? CatalogCategory(record->menuCategory) : RuntimeCategory(element)) << "\n";
	details << Localization::Ref().Tr("encyclopedia.state") << ": "
		<< (record ? CatalogValue("state", record->elementState) : RuntimeState(element)) << "\n";
	details << Localization::Ref().Tr("encyclopedia.heat_conductivity") << ": "
		<< int(element.HeatConduct) << "\n";
	details << Localization::Ref().Tr("encyclopedia.heat_capacity") << ": "
		<< element.HeatCapacity << "\n";
	if (element.LowTemperatureTransition != NT)
		details << Localization::Ref().Tr("encyclopedia.low_temperature") << ": "
			<< element.LowTemperature << " K\n";
	if (element.HighTemperatureTransition != NT)
		details << Localization::Ref().Tr("encyclopedia.high_temperature") << ": "
			<< element.HighTemperature << " K\n";

	if (record)
	{
		auto appendContent = [&](char const *key, std::string_view english, std::string_view chinese) {
			auto content = CatalogContent(language, english, chinese);
			if (!content.empty())
				details << "\n" << Localization::Ref().Tr(key) << "\n" << content << "\n";
		};
		appendContent("encyclopedia.recipe", record->recipeEnglish, record->recipeChinese);
		appendContent("encyclopedia.production", record->productionEnglish, record->productionChinese);
		appendContent("encyclopedia.use", record->useEnglish, record->useChinese);
		appendContent("encyclopedia.hazard", record->hazardEnglish, record->hazardChinese);
	}

	new InformationMessage(title, details.Build(), true);
}
