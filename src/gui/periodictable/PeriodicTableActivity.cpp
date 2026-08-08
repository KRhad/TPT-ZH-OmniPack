#include "PeriodicTableActivity.h"
#include "PeriodicElementDetailActivity.h"

#include "common/Localization.h"
#include "graphics/Graphics.h"
#include "gui/Style.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/game/GameController.h"
#include "gui/game/OmniContent.h"
#include "gui/game/tool/Tool.h"
#include "gui/interface/Button.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/PeriodicContentLinks.h"
#include "simulation/PeriodicTableData.h"

#include <SDL.h>
#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace
{
constexpr int GridX = 12;
constexpr int GridY = 62;
constexpr int CellWidth = 32;
constexpr int CellHeight = 28;
constexpr int SeriesGap = 6;

String Utf8(std::string_view value)
{
	return ByteString(value.data(), value.size()).FromUtf8();
}

String LocalizedValue(char const *prefix, std::string_view value)
{
	ByteString key(prefix);
	key.append(value.data(), value.size());
	auto fallback = ByteString(value.data(), value.size());
	return Localization::Ref().Tr(key.c_str(), fallback.c_str());
}

std::string_view StateName(PeriodicStandardState state)
{
	switch (state)
	{
	case PeriodicStandardState::Solid: return "solid";
	case PeriodicStandardState::Liquid: return "liquid";
	case PeriodicStandardState::Gas: return "gas";
	default: return "unknown";
	}
}

std::string_view ClassName(PeriodicMaterialClass materialClass)
{
	switch (materialClass)
	{
	case PeriodicMaterialClass::Metal: return "metal";
	case PeriodicMaterialClass::Metalloid: return "metalloid";
	case PeriodicMaterialClass::Nonmetal: return "nonmetal";
	default: return "unknown";
	}
}

ui::Colour FamilyColour(PeriodicElementRecord const &record, bool available)
{
	ui::Colour colour(70, 78, 88, 210);
	auto family = record.family;
	if (record.series == "lanthanide") colour = ui::Colour(153, 72, 142, 220);
	else if (record.series == "actinide") colour = ui::Colour(142, 70, 82, 220);
	else if (record.series == "superheavy") colour = ui::Colour(104, 70, 128, 220);
	else if (family == "hydrogen") colour = ui::Colour(82, 142, 178, 220);
	else if (family == "alkali_metal") colour = ui::Colour(178, 76, 70, 220);
	else if (family == "alkaline_earth_metal") colour = ui::Colour(182, 118, 58, 220);
	else if (family == "transition_metal") colour = ui::Colour(82, 112, 150, 220);
	else if (family == "boron_group") colour = ui::Colour(122, 102, 158, 220);
	else if (family == "carbon_group") colour = ui::Colour(70, 136, 102, 220);
	else if (family == "pnictogen") colour = ui::Colour(58, 132, 132, 220);
	else if (family == "chalcogen") colour = ui::Colour(148, 136, 62, 220);
	else if (family == "halogen") colour = ui::Colour(180, 104, 54, 220);
	else if (family == "noble_gas") colour = ui::Colour(94, 84, 178, 220);
	if (!available)
	{
		colour.Red = uint8_t(colour.Red / 3);
		colour.Green = uint8_t(colour.Green / 3);
		colour.Blue = uint8_t(colour.Blue / 3);
		colour.Alpha = 180;
	}
	return colour;
}

class PeriodicElementButton: public ui::Button
{
	int atomicNumber;
	bool available;

public:
	PeriodicElementButton(
		ui::Point position,
		ui::Point size,
		PeriodicElementRecord const &record,
		bool available,
		String toolTip):
		ui::Button(position, size, Utf8(record.symbol), std::move(toolTip)),
		atomicNumber(record.atomicNumber),
		available(available)
	{
		Appearance.BackgroundInactive = FamilyColour(record, available);
		Appearance.BackgroundHover = FamilyColour(record, true);
		Appearance.BackgroundActive = ui::Colour(215, 215, 215, 220);
		Appearance.BorderInactive = available
			? ui::Colour(175, 175, 175, 255)
			: ui::Colour(76, 76, 76, 255);
		Appearance.TextInactive = available
			? ui::Colour(255, 255, 255, 255)
			: ui::Colour(145, 145, 145, 255);
		Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
		Appearance.VerticalAlign = ui::Appearance::AlignBottom;
	}

	void Draw(ui::Point const &screenPos) override
	{
		ui::Button::Draw(screenPos);
		auto colour = available
			? ui::Colour(230, 230, 230, 255)
			: ui::Colour(110, 110, 110, 255);
		GetGraphics()->BlendText(screenPos + ui::Point(2, 1), String::Build(atomicNumber), colour);
	}
};

bool MatchesState(PeriodicElementRecord const &record, int filter)
{
	return filter == 0 || int(record.standardState) == filter - 1;
}

bool MatchesClass(PeriodicElementRecord const &record, int filter)
{
	return filter == 0 || int(record.materialClass) == filter - 1;
}

bool MatchesRadioactivity(PeriodicElementRecord const &record, int filter)
{
	return filter == 0 || (filter == 1 && record.radioactive) ||
		(filter == 2 && !record.radioactive);
}
}

PeriodicTableActivity::PeriodicTableActivity(
	GameController *gameController,
	std::vector<Tool *> tools):
	WindowActivity(ui::Point(-1, -1), ui::Point(600, 365)),
	gameController(gameController),
	tools(std::move(tools))
{
	auto *title = new ui::Label(
		ui::Point(5, 2), ui::Point(Size.X - 10, 15),
		Localization::Ref().Tr("periodic.table.title"));
	title->SetTextColour(style::Colour::InformationTitle);
	title->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(title);

	searchField = new ui::Textbox(
		ui::Point(8, 20), ui::Point(526, 17), "",
		Localization::Ref().Tr("periodic.table.search_placeholder"));
	searchField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	searchField->SetActionCallback({ [this] { RebuildElements(); } });
	AddComponent(searchField);

	stateFilterButton = new ui::Button(ui::Point(8, 40), ui::Point(136, 17));
	stateFilterButton->SetActionCallback({ [this] {
		stateFilter = (stateFilter + 1) % 5;
		RefreshFilterLabels();
		RebuildElements();
	} });
	AddComponent(stateFilterButton);

	classFilterButton = new ui::Button(ui::Point(148, 40), ui::Point(136, 17));
	classFilterButton->SetActionCallback({ [this] {
		classFilter = (classFilter + 1) % 5;
		RefreshFilterLabels();
		RebuildElements();
	} });
	AddComponent(classFilterButton);

	radioactivityFilterButton = new ui::Button(ui::Point(288, 40), ui::Point(136, 17));
	radioactivityFilterButton->SetActionCallback({ [this] {
		radioactivityFilter = (radioactivityFilter + 1) % 3;
		RefreshFilterLabels();
		RebuildElements();
	} });
	AddComponent(radioactivityFilterButton);

	seriesButton = new ui::Button(ui::Point(428, 40), ui::Point(164, 17));
	seriesButton->SetActionCallback({ [this] {
		showSeries = !showSeries;
		RefreshFilterLabels();
		RebuildElements();
	} });
	AddComponent(seriesButton);

	auto *closeButton = new ui::Button(
		ui::Point(538, 20), ui::Point(54, 17),
		Localization::Ref().Tr("periodic.table.close"));
	closeButton->SetActionCallback({ [this] { exit = true; } });
	AddComponent(closeButton);

	statusLabel = new ui::Label(
		ui::Point(8, Size.Y - 24), ui::Point(Size.X - 16, 16),
		Localization::Ref().Tr("periodic.table.hover_hint"));
	statusLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	statusLabel->SetTextColour(ui::Colour(205, 205, 205, 255));
	AddComponent(statusLabel);

	RefreshFilterLabels();
	RebuildElements();
	FocusComponent(searchField);
}

PeriodicTableActivity::~PeriodicTableActivity() = default;

Tool *PeriodicTableActivity::FindTool(std::string_view identifier) const
{
	for (auto *tool : tools)
	{
		if (!tool)
		{
			continue;
		}
		auto toolIdentifier = std::string_view(tool->Identifier.data(), tool->Identifier.size());
		if (toolIdentifier == identifier)
		{
			return tool;
		}
	}
	return nullptr;
}

void PeriodicTableActivity::RefreshFilterLabels()
{
	static constexpr char const *stateKeys[] = {
		"periodic.filter.state.all", "periodic.filter.state.solid",
		"periodic.filter.state.liquid", "periodic.filter.state.gas",
		"periodic.filter.state.unknown",
	};
	static constexpr char const *classKeys[] = {
		"periodic.filter.class.all", "periodic.filter.class.metal",
		"periodic.filter.class.metalloid", "periodic.filter.class.nonmetal",
		"periodic.filter.class.unknown",
	};
	static constexpr char const *radioactivityKeys[] = {
		"periodic.filter.radioactivity.all", "periodic.filter.radioactivity.radioactive",
		"periodic.filter.radioactivity.nonradioactive",
	};
	stateFilterButton->SetText(Localization::Ref().Tr(stateKeys[stateFilter]));
	classFilterButton->SetText(Localization::Ref().Tr(classKeys[classFilter]));
	radioactivityFilterButton->SetText(
		Localization::Ref().Tr(radioactivityKeys[radioactivityFilter]));
	seriesButton->SetText(Localization::Ref().Tr(
		showSeries ? "periodic.series.hide" : "periodic.series.show"));
}

void PeriodicTableActivity::RebuildElements()
{
	for (auto *button : elementButtons)
	{
		RemoveComponent(button);
		delete button;
	}
	elementButtons.clear();

	auto query = searchField->GetText().ToLower();
	std::map<int, PeriodicContentLink const *> relatedSearchMatches;
	if (!query.empty())
	{
		for (auto const &link : GetPeriodicContentLinks())
		{
			if (!link.periodicVisible || link.contentKind == PeriodicContentKind::PeriodicElement)
				continue;
			auto searchable = String::Build(
				Utf8(link.formula), " ", Utf8(link.chineseName), " ",
				Utf8(link.englishName), " ", Utf8(link.toolIdentifier)).ToLower();
			if (!searchable.Contains(query))
				continue;
			for (int atomicNumber = 1; atomicNumber <= 118; ++atomicNumber)
			{
				if (PeriodicContentRelatesTo(link, atomicNumber))
					relatedSearchMatches.try_emplace(atomicNumber, &link);
			}
		}
	}
	if (!query.empty() && !relatedSearchMatches.empty())
	{
		auto const *match = relatedSearchMatches.begin()->second;
		auto chineseInterface = GlobalPrefs::Ref().Get("Language", 1) == 1;
		auto relatedSeparator = chineseInterface ? String("：[") : String(": [");
		statusLabel->SetText(String::Build(
			Localization::Ref().Tr("periodic.search.related_material"),
			relatedSeparator,
			Utf8(match->formula), "] ",
			chineseInterface ? Utf8(match->chineseName) : Utf8(match->englishName)));
	}
	else
	{
		statusLabel->SetText(Localization::Ref().Tr("periodic.table.hover_hint"));
	}

	for (auto const &record : GetPeriodicTableData())
	{
		if (!showSeries && record.tableRow >= 7)
		{
			continue;
		}
		if (!MatchesState(record, stateFilter) || !MatchesClass(record, classFilter) ||
			!MatchesRadioactivity(record, radioactivityFilter))
		{
			continue;
		}

		if (!query.empty())
		{
			auto searchable = String::Build(
				record.atomicNumber, " ", Utf8(record.symbol), " ",
				Utf8(record.chineseName), " ", Utf8(record.englishName), " ",
				Utf8(record.identifier)).ToLower();
			if (!searchable.Contains(query) && !relatedSearchMatches.contains(record.atomicNumber))
			{
				continue;
			}
		}

		auto *tool = record.implemented ? FindTool(record.identifier) : nullptr;
		bool selectable = tool && IsOmniToolSelectable(*tool);
		String status;
		if (!record.implemented || !tool)
		{
			status = Localization::Ref().Tr("periodic.status.planned");
		}
		else if (!selectable)
		{
			status = Localization::Ref().Tr("periodic.status.module_disabled");
		}
		auto chineseInterface = GlobalPrefs::Ref().Get("Language", 1) == 1;
		auto primaryName = chineseInterface ? Utf8(record.chineseName) : Utf8(record.englishName);
		auto secondaryName = chineseInterface ? Utf8(record.englishName) : Utf8(record.chineseName);
		auto details = String::Build(
			record.atomicNumber, " ", Utf8(record.symbol), " — ",
			primaryName, " (", secondaryName, ") — ",
			LocalizedValue("periodic.family.", record.family), " — ",
			LocalizedValue("periodic.class.", ClassName(record.materialClass)), " — ",
			LocalizedValue("periodic.state.", StateName(record.standardState)));
		auto related = relatedSearchMatches.find(record.atomicNumber);
		if (related != relatedSearchMatches.end())
		{
			auto const *match = related->second;
			auto relatedSeparator = chineseInterface ? String("：[") : String(": [");
			details += String::Build(
				" — ", Localization::Ref().Tr("periodic.search.related_material"),
				relatedSeparator, Utf8(match->formula), "] ",
				chineseInterface ? Utf8(match->chineseName) : Utf8(match->englishName));
		}
		if (!status.empty())
		{
			details += String::Build(" — ", status);
		}

		int extraGap = record.tableRow >= 7 ? SeriesGap : 0;
		auto const *recordPointer = &record;
		auto *button = new PeriodicElementButton(
			ui::Point(
				GridX + record.tableColumn * CellWidth,
				GridY + record.tableRow * CellHeight + extraGap),
			ui::Point(CellWidth - 1, CellHeight - 1), record, selectable, details);
		std::string highlightedIdentifier;
		if (related != relatedSearchMatches.end())
			highlightedIdentifier = std::string(related->second->toolIdentifier);
		button->SetActionCallback({
			[this, recordPointer, highlightedIdentifier] {
				new PeriodicElementDetailActivity(
					gameController, tools, *recordPointer, highlightedIdentifier,
					[this](Tool *selectedTool) {
						gameController->SetActiveTool(0, selectedTool);
						gameController->ShowElementDescription(selectedTool);
						exit = true;
					});
			},
			nullptr,
			[this, details] { statusLabel->SetText(details); },
		});
		AddComponent(button);
		elementButtons.push_back(button);
	}
}

void PeriodicTableActivity::OnDraw()
{
	auto *graphics = GetGraphics();
	graphics->DrawFilledRect(RectSized(Position - Vec2{ 1, 1 }, Size + Vec2{ 2, 2 }), 0x000000_rgb);
	graphics->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);
	graphics->BlendText(
		Position + ui::Point(GridX, GridY + 7 * CellHeight + SeriesGap + 7),
		Localization::Ref().Tr("periodic.series.lanthanides"),
		ui::Colour(180, 180, 180, showSeries ? 255 : 0));
	graphics->BlendText(
		Position + ui::Point(GridX, GridY + 8 * CellHeight + SeriesGap + 7),
		Localization::Ref().Tr("periodic.series.actinides"),
		ui::Colour(180, 180, 180, showSeries ? 255 : 0));
}

void PeriodicTableActivity::OnTick()
{
	if (exit)
	{
		Exit();
	}
}

void PeriodicTableActivity::OnKeyPress(
	int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (repeat)
	{
		return;
	}
	if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
	{
		exit = true;
	}
}
