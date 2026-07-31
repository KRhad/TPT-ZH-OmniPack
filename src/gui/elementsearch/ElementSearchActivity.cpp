#include "ElementSearchActivity.h"
#include "ElementCatalog.h"

#include <set>
#include <map>
#include <algorithm>
#include <SDL.h>

#include "common/Localization.h"
#include "prefs/GlobalPrefs.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/ScrollPanel.h"
#include "gui/interface/Label.h"
#include "gui/game/tool/Tool.h"
#include "gui/game/Menu.h"
#include "gui/Style.h"
#include "gui/game/Favorite.h"
#include "gui/game/GameController.h"
#include "gui/game/OmniContent.h"
#include "gui/game/ToolButton.h"
#include "gui/dialogues/InformationMessage.h"

#include "graphics/Graphics.h"
#include "simulation/SimulationData.h"

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
	};
	for (auto const &[value, key] : categoryKeys)
	{
		if (category == value)
		{
			return Localization::Ref().Tr(key);
		}
	}
	return CatalogValue("category", category);
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

ElementSearchActivity::ElementSearchActivity(GameController * gameController, std::vector<Tool*> tools) :
	WindowActivity(ui::Point(-1, -1), ui::Point(236, 302)),
	firstResult(nullptr),
	gameController(gameController),
	tools(tools),
	toolTip(""),
	shiftPressed(false),
	ctrlPressed(false),
	altPressed(false),
	isToolTipFadingIn(false),
	exit(false)
{
	ui::Label * title = new ui::Label(ui::Point(4, 5), ui::Point(Size.X-8, 15), Localization::Ref().Tr("elementsearch.title"));
	title->SetTextColour(style::Colour::InformationTitle);
	title->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(title);

	searchField = new ui::Textbox(ui::Point(8, 23), ui::Point(Size.X-16, 17), "");
	searchField->SetActionCallback({ [this] { searchTools(searchField->GetText()); } });
	searchField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(searchField);
	FocusComponent(searchField);

	auto thirdWidth = Size.X / 3;
	ui::Button * closeButton = new ui::Button(ui::Point(0, Size.Y-15), ui::Point(thirdWidth+1, 15), Localization::Ref().Tr("elementsearch.close"));
	closeButton->SetActionCallback({ [this] { exit = true; } });
	ui::Button * encyclopediaButton = new ui::Button(ui::Point(thirdWidth, Size.Y-15), ui::Point(thirdWidth+1, 15), Localization::Ref().Tr("elementsearch.encyclopedia"));
	encyclopediaButton->SetActionCallback({ [this] {
		auto *tool = GetFirstResult();
		auto const *record = tool ? FindElementCatalogByIdentifier(tool->Identifier) : nullptr;
		if (!tool || !record)
		{
			new InformationMessage(
				Localization::Ref().Tr("elementsearch.encyclopedia"),
				Localization::Ref().Tr("elementsearch.encyclopedia_unavailable"),
				false
			);
			return;
		}

		auto language = GlobalPrefs::Ref().Get("Language", 1);
		auto currentName = language == 1 && !record->chineseName.empty()
			? CatalogString(record->chineseName)
			: CatalogString(record->englishName);
		auto title = currentName;
		if (language == 1 && record->chineseName != record->englishName)
		{
			title += " / " + CatalogString(record->englishName);
		}
		title += " [" + CatalogString(record->displayCode) + "]";

		StringBuilder details;
		details << Localization::Ref().Tr("encyclopedia.identifier") << ": " << CatalogString(record->identifier) << "\n";
		details << Localization::Ref().Tr("encyclopedia.stable_id") << ": " << record->stableId << "\n";
		details << Localization::Ref().Tr("encyclopedia.category") << ": " << CatalogCategory(record->menuCategory) << "\n";
		details << Localization::Ref().Tr("encyclopedia.state") << ": " << CatalogValue("state", record->elementState) << "\n";
		details << Localization::Ref().Tr("encyclopedia.source") << ": " << CatalogString(record->sourceMod) << "\n";
		details << Localization::Ref().Tr("encyclopedia.source_commit") << ": " << CatalogString(record->sourceCommit) << "\n";
		details << Localization::Ref().Tr("encyclopedia.license") << ": " << CatalogString(record->license) << "\n";
		details << Localization::Ref().Tr("encyclopedia.save_compatibility") << ": " << CatalogValue("save", record->saveCompatibility) << "\n";
		details << Localization::Ref().Tr("encyclopedia.implementation") << ": " << CatalogValue("implementation", record->implementationStatus) << "\n";
		details << Localization::Ref().Tr("encyclopedia.test_status") << ": " << CatalogValue("test", record->testStatus) << "\n";
		auto appendContent = [&](char const *key, std::string_view english, std::string_view chinese) {
			auto content = CatalogContent(language, english, chinese);
			if (!content.empty())
				details << Localization::Ref().Tr(key) << ": " << content << "\n";
		};
		appendContent("encyclopedia.recipe", record->recipeEnglish, record->recipeChinese);
		appendContent("encyclopedia.production", record->productionEnglish, record->productionChinese);
		appendContent("encyclopedia.use", record->useEnglish, record->useChinese);
		appendContent("encyclopedia.hazard", record->hazardEnglish, record->hazardChinese);

		if (record->stableId >= 0 && record->stableId < PT_NUM)
		{
			auto const &element = SimulationData::Ref().elements[record->stableId];
			details << Localization::Ref().Tr("encyclopedia.heat_conductivity") << ": " << int(element.HeatConduct) << "\n";
			details << Localization::Ref().Tr("encyclopedia.heat_capacity") << ": " << element.HeatCapacity << "\n";
			details << Localization::Ref().Tr("encyclopedia.low_temperature") << ": " << element.LowTemperature << " K\n";
			details << Localization::Ref().Tr("encyclopedia.high_temperature") << ": " << element.HighTemperature << " K\n\n";
			auto catalogDescription = language == 1
				? CatalogString(record->chineseDescription)
				: CatalogString(record->englishDescription);
			details << Localization::Ref().Tr("encyclopedia.description") << ": "
				<< (catalogDescription.empty() ? element.Description : catalogDescription);
		}
		new InformationMessage(title, details.Build(), true);
	} });
	ui::Button * okButton = new ui::Button(ui::Point(thirdWidth*2, Size.Y-15), ui::Point(Size.X-thirdWidth*2, 15), Localization::Ref().Tr("dialog.ok"));
	okButton->SetActionCallback({ [this] {
		if (GetFirstResult())
			SetActiveTool(0, GetFirstResult());
	} });

	AddComponent(okButton);
	AddComponent(encyclopediaButton);
	AddComponent(closeButton);

	scrollPanel = new ui::ScrollPanel(searchField->Position + Vec2{ 1, searchField->Size.Y+9 }, { searchField->Size.X - 2, Size.Y-(searchField->Position.Y+searchField->Size.Y+6)-23 });
	AddComponent(scrollPanel);

	searchTools("");
}

void ElementSearchActivity::searchTools(String query)
{
	firstResult = nullptr;
	for (auto &toolButton : toolButtons) {
		scrollPanel->RemoveChild(toolButton);
		delete toolButton;
	}
	toolButtons.clear();

	ui::Point viewPosition = { 1, 1 };
	ui::Point current = ui::Point(0, 0);

	String queryLower = query.ToLower();

	struct Match
	{
		int toolIndex; // relevance by position of tool in tools vector
		int haystackOrigin; // relevance by origin of haystack
		int needlePosition; // relevance by position of needle in haystack

		bool operator <(Match const &other) const
		{
			return std::tie(haystackOrigin, needlePosition, toolIndex) < std::tie(other.haystackOrigin, other.needlePosition, other.toolIndex);
		}
	};

	std::map<int, Match> indexToMatch;
	auto push = [ &indexToMatch ](Match match) {
		auto it = indexToMatch.find(match.toolIndex);
		if (it == indexToMatch.end())
		{
			indexToMatch.insert(std::make_pair(match.toolIndex, match));
		}
		else if (match < it->second)
		{
			it->second = match;
		}
	};

	auto pushIfMatches = [ &queryLower, &push ](String infoLower, int toolIndex, int haystackRelevance) {
		if (infoLower == queryLower)
		{
			push(Match{ toolIndex, haystackRelevance, 0 });
		}
		if (infoLower.BeginsWith(queryLower))
		{
			push(Match{ toolIndex, haystackRelevance, 1 });
		}
		if (infoLower.Contains(queryLower))
		{
			push(Match{ toolIndex, haystackRelevance, 2 });
		}
	};

	std::map<Tool *, String> menudescriptionLower;
	for (auto *menu : gameController->GetMenuList())
	{
		for (auto *tool : menu->GetToolList())
		{
			menudescriptionLower.insert(std::make_pair(tool, menu->GetDescription().ToLower()));
		}
	}

	for (int toolIndex = 0; toolIndex < (int)tools.size(); ++toolIndex)
	{
		if (!IsOmniToolSelectable(*tools[toolIndex]))
		{
			continue;
		}
		pushIfMatches(tools[toolIndex]->Name.ToLower(), toolIndex, 0);
		pushIfMatches(tools[toolIndex]->Identifier.FromUtf8().ToLower(), toolIndex, 1);
		if (auto const *record = FindElementCatalogByIdentifier(tools[toolIndex]->Identifier))
		{
			pushIfMatches(CatalogString(record->displayCode).ToLower(), toolIndex, 0);
			pushIfMatches(CatalogString(record->chineseName).ToLower(), toolIndex, 1);
			pushIfMatches(CatalogString(record->englishName).ToLower(), toolIndex, 1);
			pushIfMatches(CatalogString(record->sourceMod).ToLower(), toolIndex, 2);
			pushIfMatches(CatalogString(record->menuCategory).ToLower(), toolIndex, 2);
			pushIfMatches(CatalogString(record->englishDescription).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->chineseDescription).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->recipeEnglish).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->recipeChinese).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->productionEnglish).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->productionChinese).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->useEnglish).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->useChinese).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->hazardEnglish).ToLower(), toolIndex, 3);
			pushIfMatches(CatalogString(record->hazardChinese).ToLower(), toolIndex, 3);
		}
		pushIfMatches(tools[toolIndex]->Description.ToLower(), toolIndex, 3);
		auto it = menudescriptionLower.find(tools[toolIndex]);
		if (it != menudescriptionLower.end())
		{
			pushIfMatches(it->second, toolIndex, 4);
		}
	}

	std::vector<Match> matches;
	std::transform(indexToMatch.begin(), indexToMatch.end(), std::back_inserter(matches), [](decltype(indexToMatch)::value_type const &pair) {
		return pair.second;
	});
	std::sort(matches.begin(), matches.end());
	for (auto &match : matches)
	{
		Tool *tool = tools[match.toolIndex];

		if(!firstResult)
			firstResult = tool;

		std::unique_ptr<VideoBuffer> tempTexture = tool->GetTexture(Vec2(26, 14));
		ToolButton * tempButton;

		if(tempTexture)
			tempButton = new ToolButton(current+viewPosition, ui::Point(30, 18), "", tool->Identifier, tool->Description);
		else
			tempButton = new ToolButton(current+viewPosition, ui::Point(30, 18), tool->Name, tool->Identifier, tool->Description);

		tempButton->Appearance.SetTexture(std::move(tempTexture));
		tempButton->Appearance.BackgroundInactive = tool->Colour.WithAlpha(0xFF);
		tempButton->SetActionCallback({ [this, tempButton, tool] {
			if (tempButton->GetSelectionState() >= 0 && tempButton->GetSelectionState() <= 2)
				SetActiveTool(tempButton->GetSelectionState(), tool);
		} });

		if(gameController->GetActiveTool(0) == tool)
		{
			tempButton->SetSelectionState(0);	//Primary
		}
		else if(gameController->GetActiveTool(1) == tool)
		{
			tempButton->SetSelectionState(1);	//Secondary
		}
		else if(gameController->GetActiveTool(2) == tool)
		{
			tempButton->SetSelectionState(2);	//Tertiary
		}

		toolButtons.push_back(tempButton);
		scrollPanel->AddChild(tempButton);

		current.X += 31;

		if(current.X + 30 > searchField->Size.X) {
			current.X = 0;
			current.Y += 19;
		}
	}

	if (current.X == 0)
	{
		current.Y -= 19;
	}
	scrollPanel->InnerSize = ui::Point(scrollPanel->Size.X, current.Y + 20);
}

void ElementSearchActivity::SetActiveTool(int selectionState, Tool * tool)
{
	if (ctrlPressed && shiftPressed && !altPressed)
	{
		Favorite::Ref().AddFavorite(tool->Identifier);
		gameController->RebuildFavoritesMenu();
	}
	else if (ctrlPressed && altPressed && !shiftPressed &&
	         tool->Identifier.BeginsWith("DEFAULT_PT_"))
	{
		gameController->SetActiveTool(3, tool);
	}
	else
		gameController->SetActiveTool(selectionState, tool);
	exit = true;
}

void ElementSearchActivity::OnDraw()
{
	Graphics * g = GetGraphics();
	g->DrawFilledRect(RectSized(Position - Vec2{ 1, 1 }, Size + Vec2{ 2, 2 }), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);

	g->BlendRect(
		RectSized(Position + scrollPanel->Position - Vec2{ 1, 1 }, scrollPanel->Size + Vec2{ 2, 2 }),
		0xFFFFFF_rgb .WithAlpha(180));
	if (toolTipPresence && toolTip.length())
	{
		g->BlendText({ 10, Size.Y+70 }, toolTip, 0xFFFFFF_rgb .WithAlpha(toolTipPresence>51?255:toolTipPresence*5));
	}
}

void ElementSearchActivity::OnTick()
{
	if (exit)
		Exit();

	if (isToolTipFadingIn)
	{
		isToolTipFadingIn = false;
		toolTipPresence.SetTarget(120);
	}
	else
	{
		toolTipPresence.SetTarget(0);
	}
}

void ElementSearchActivity::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (repeat)
		return;
	switch (key)
	{
	case SDLK_KP_ENTER:
	case SDLK_RETURN:
		if(firstResult)
			gameController->SetActiveTool(0, firstResult);
	case SDLK_ESCAPE:
	case SDLK_AC_BACK:
		exit = true;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		shiftPressed = true;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		ctrlPressed = true;
		break;
	case SDLK_LALT:
	case SDLK_RALT:
		altPressed = true;
		break;
	}
}

void ElementSearchActivity::OnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (repeat)
		return;
	switch (key)
	{
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		shiftPressed = false;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		ctrlPressed = false;
		break;
	case SDLK_LALT:
	case SDLK_RALT:
		altPressed = false;
		break;
	}
}

void ElementSearchActivity::ToolTip(ui::Point senderPosition, String toolTip)
{
	this->toolTip = toolTip;
	this->isToolTipFadingIn = true;
}

ElementSearchActivity::~ElementSearchActivity() {
}

