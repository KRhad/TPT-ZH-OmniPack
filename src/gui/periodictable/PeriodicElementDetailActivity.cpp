#include "PeriodicElementDetailActivity.h"

#include "common/Localization.h"
#include "graphics/Graphics.h"
#include "gui/Style.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/elementsearch/ElementInfo.h"
#include "gui/game/GameController.h"
#include "gui/game/OmniContent.h"
#include "gui/game/tool/Tool.h"
#include "gui/interface/Button.h"
#include "gui/interface/Label.h"
#include "gui/interface/ScrollPanel.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/ElementDefs.h"
#include "simulation/PeriodicContentLinks.h"

#include "common/platform/SDLCompat.h"
#include <algorithm>
#include <set>
#include <string>
#include <utility>

namespace
{
String Utf8(std::string_view value)
{
	return ByteString(value.data(), value.size()).FromUtf8();
}

char const *GroupKey(PeriodicCompoundGroup group)
{
	switch (group)
	{
	case PeriodicCompoundGroup::Element: return "periodic.detail.element";
	case PeriodicCompoundGroup::Allotrope: return "periodic.detail.allotrope";
	case PeriodicCompoundGroup::Isotope: return "periodic.detail.isotope";
	case PeriodicCompoundGroup::Oxide: return "periodic.detail.oxide";
	case PeriodicCompoundGroup::Hydroxide: return "periodic.detail.hydroxide";
	case PeriodicCompoundGroup::Acid: return "periodic.detail.acid";
	case PeriodicCompoundGroup::Base: return "periodic.detail.base";
	case PeriodicCompoundGroup::Salt: return "periodic.detail.salt";
	case PeriodicCompoundGroup::Halide: return "periodic.detail.halide";
	case PeriodicCompoundGroup::Sulfide: return "periodic.detail.sulfide";
	case PeriodicCompoundGroup::Nitride: return "periodic.detail.nitride";
	case PeriodicCompoundGroup::Carbide: return "periodic.detail.carbide";
	case PeriodicCompoundGroup::Hydride: return "periodic.detail.hydride";
	case PeriodicCompoundGroup::Organic: return "periodic.detail.organic";
	case PeriodicCompoundGroup::Polymer: return "periodic.detail.polymer";
	case PeriodicCompoundGroup::Alloy: return "periodic.detail.alloy";
	case PeriodicCompoundGroup::Mineral: return "periodic.detail.mineral";
	case PeriodicCompoundGroup::Ceramic: return "periodic.detail.ceramic";
	case PeriodicCompoundGroup::Glass: return "periodic.detail.glass";
	case PeriodicCompoundGroup::Semiconductor: return "periodic.detail.semiconductor";
	case PeriodicCompoundGroup::Composite: return "periodic.detail.composite";
	case PeriodicCompoundGroup::Engineering: return "periodic.detail.engineering";
	case PeriodicCompoundGroup::Other: return "periodic.detail.other";
	}
	return "periodic.detail.other";
}

int GroupOrder(PeriodicCompoundGroup group)
{
	switch (group)
	{
	case PeriodicCompoundGroup::Element: return 0;
	case PeriodicCompoundGroup::Allotrope: return 1;
	case PeriodicCompoundGroup::Isotope: return 2;
	case PeriodicCompoundGroup::Oxide: return 3;
	case PeriodicCompoundGroup::Hydroxide: return 4;
	case PeriodicCompoundGroup::Acid: return 5;
	case PeriodicCompoundGroup::Base: return 6;
	case PeriodicCompoundGroup::Salt: return 7;
	case PeriodicCompoundGroup::Halide: return 8;
	case PeriodicCompoundGroup::Sulfide: return 9;
	case PeriodicCompoundGroup::Nitride: return 10;
	case PeriodicCompoundGroup::Carbide: return 11;
	case PeriodicCompoundGroup::Hydride: return 12;
	case PeriodicCompoundGroup::Organic: return 13;
	case PeriodicCompoundGroup::Polymer: return 14;
	case PeriodicCompoundGroup::Alloy: return 15;
	case PeriodicCompoundGroup::Mineral: return 16;
	case PeriodicCompoundGroup::Ceramic: return 17;
	case PeriodicCompoundGroup::Glass: return 18;
	case PeriodicCompoundGroup::Semiconductor: return 19;
	case PeriodicCompoundGroup::Composite: return 20;
	case PeriodicCompoundGroup::Engineering: return 21;
	case PeriodicCompoundGroup::Other: return 22;
	}
	return 23;
}
}

PeriodicElementDetailActivity::PeriodicElementDetailActivity(
	GameController *gameController,
	std::vector<Tool *> tools,
	PeriodicElementRecord const &element,
	std::string highlightedIdentifier,
	std::function<void(Tool *)> selectedCallback):
	WindowActivity(ui::Point(-1, -1), ui::Point(420, 350)),
	gameController(gameController),
	tools(std::move(tools)),
	element(element),
	highlightedIdentifier(std::move(highlightedIdentifier)),
	selectedCallback(std::move(selectedCallback))
{
	auto chineseInterface = GlobalPrefs::Ref().Get("Language", 1) == 1;
	std::set<std::string_view> relatedMaterials;
	for (auto const &link : GetPeriodicContentLinks())
	{
		if (link.periodicVisible && PeriodicContentRelatesTo(link, element.atomicNumber))
			relatedMaterials.insert(link.toolIdentifier);
	}
	auto primaryName = chineseInterface ? Utf8(element.chineseName) : Utf8(element.englishName);
	auto secondaryName = chineseInterface ? Utf8(element.englishName) : Utf8(element.chineseName);
	auto titleText = chineseInterface
		? String::Build(primaryName, "（", Utf8(element.symbol), "）")
		: String::Build(primaryName, " (", Utf8(element.symbol), ")");
	auto subtitleText = String::Build(
		secondaryName, " · ", Localization::Ref().Tr("periodic.detail.atomic_number"),
		" ", element.atomicNumber, " · ",
		Localization::Ref().Tr("periodic.detail.material_count"), " ",
		relatedMaterials.size());

	auto *title = new ui::Label(ui::Point(8, 4), ui::Point(Size.X - 78, 17), titleText);
	title->SetTextColour(style::Colour::InformationTitle);
	title->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(title);

	auto *subtitle = new ui::Label(
		ui::Point(8, 21), ui::Point(Size.X - 78, 15), subtitleText);
	subtitle->SetTextColour(ui::Colour(170, 170, 170, 255));
	subtitle->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(subtitle);

	auto *backButton = new ui::Button(
		ui::Point(Size.X - 66, 5), ui::Point(58, 17),
		Localization::Ref().Tr("periodic.detail.back"));
	backButton->SetActionCallback({ [this] { exit = true; } });
	AddComponent(backButton);

	BuildContent();
}

PeriodicElementDetailActivity::~PeriodicElementDetailActivity() = default;

Tool *PeriodicElementDetailActivity::FindTool(std::string_view identifier) const
{
	for (auto *tool : tools)
	{
		if (!tool)
			continue;
		auto toolIdentifier = std::string_view(tool->Identifier.data(), tool->Identifier.size());
		if (toolIdentifier == identifier)
			return tool;
	}
	return nullptr;
}

void PeriodicElementDetailActivity::BuildContent()
{
	std::vector<PeriodicContentLink const *> links;
	for (auto const &link : GetPeriodicContentLinks())
	{
		if (link.periodicVisible && PeriodicContentRelatesTo(link, element.atomicNumber))
			links.push_back(&link);
	}
	std::stable_sort(links.begin(), links.end(), [](auto const *left, auto const *right) {
		return std::pair(GroupOrder(left->compoundGroup), left->displayOrder)
			< std::pair(GroupOrder(right->compoundGroup), right->displayOrder);
	});

	auto chineseInterface = GlobalPrefs::Ref().Get("Language", 1) == 1;
	auto *longPressHint = new ui::Label(
		ui::Point(8, 42), ui::Point(Size.X - 16, 17),
		String::Build("· ", Localization::Ref().Tr("element.long_press_hint")));
	longPressHint->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	longPressHint->SetTextColour(chineseInterface
		? ui::Colour(255, 255, 255, 255)
		: ui::Colour(205, 205, 205, 255));
	AddComponent(longPressHint);

	auto materialsTop = longPressHint->Position.Y + longPressHint->Size.Y + 6;
	contentPanel = new ui::ScrollPanel(
		ui::Point(8, materialsTop), ui::Point(Size.X - 16, Size.Y - materialsTop - 8));
	AddComponent(contentPanel);
	int y = 0;

	std::set<std::string_view> identifiers;
	PeriodicCompoundGroup previousGroup = PeriodicCompoundGroup::Other;
	bool haveGroup = false;
	int materialColumn = 0;
	constexpr int MaterialColumns = 2;
	constexpr int MaterialGap = 4;
	auto materialButtonWidth =
		(contentPanel->Size.X - 10 - MaterialGap) / MaterialColumns;
	int highlightedY = -1;
	for (auto const *link : links)
	{
		if (!identifiers.insert(link->toolIdentifier).second)
			continue;
		if (!haveGroup || link->compoundGroup != previousGroup)
		{
			if (materialColumn)
			{
				y += 24;
				materialColumn = 0;
			}
			auto *groupLabel = new ui::Label(
				ui::Point(3, y), ui::Point(contentPanel->Size.X - 14, 17),
				Localization::Ref().Tr(GroupKey(link->compoundGroup)));
			groupLabel->SetTextColour(style::Colour::InformationTitle);
			groupLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
			contentPanel->AddChild(groupLabel);
			y += 18;
			previousGroup = link->compoundGroup;
			haveGroup = true;
		}

		auto *tool = FindTool(link->toolIdentifier);
		auto restriction = tool && tool->IsElement
			? GetOmniElementSelectionRestriction(TYP(tool->ToolID))
			: OmniSelectionRestriction::InvalidElement;
		auto selectable = tool
			&& restriction == OmniSelectionRestriction::None
			&& IsOmniToolSelectable(*tool);
		auto primaryName = chineseInterface ? Utf8(link->chineseName) : Utf8(link->englishName);
		auto secondaryName = chineseInterface ? Utf8(link->englishName) : Utf8(link->chineseName);
		auto buttonText = String::Build("[", Utf8(link->formula), "] ", primaryName);
		auto toolTip = String::Build(secondaryName);
		if (tool && !tool->Description.empty())
			toolTip += String::Build(" · ", tool->Description);

		auto *button = new ui::Button(
			ui::Point(3 + materialColumn * (materialButtonWidth + MaterialGap), y),
			ui::Point(materialButtonWidth, 22),
			buttonText, toolTip);
		button->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		button->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		if (!selectable)
			button->Appearance.TextInactive = ui::Colour(135, 135, 135, 255);
		if (link->toolIdentifier == highlightedIdentifier)
		{
			button->Appearance.BackgroundInactive = ui::Colour(48, 76, 108, 255);
			button->Appearance.BorderInactive = ui::Colour(125, 180, 225, 255);
			highlightedY = y;
		}

		auto title = String::Build(primaryName, " [", Utf8(link->formula), "]");
		button->SetActionCallback({ [this, tool, restriction, selectable, title] {
			if (!tool)
			{
				new InformationMessage(
					title, Localization::Ref().Tr("periodic.detail.not_implemented"), false);
				return;
			}
			if (restriction == OmniSelectionRestriction::ModuleDisabled)
			{
				auto *controller = gameController;
				new ConfirmPrompt(
					Localization::Ref().Tr("periodic.detail.module_disabled_title"),
					Localization::Ref().Tr("periodic.detail.module_disabled"),
					{ [controller] { controller->OpenOptions(); } },
					Localization::Ref().Tr("periodic.detail.module_settings"));
				return;
			}
			if (!selectable)
			{
				new InformationMessage(
					title, Localization::Ref().Tr("periodic.detail.not_implemented"), false);
				return;
			}
			if (selectedCallback)
				selectedCallback(tool);
			exit = true;
		} });
		if (tool)
			button->SetLongPressCallback([tool] { OpenElementInfo(tool); });
		contentPanel->AddChild(button);
		materialColumn = (materialColumn + 1) % MaterialColumns;
		if (!materialColumn)
			y += 24;
	}
	if (materialColumn)
		y += 24;
	contentPanel->InnerSize = ui::Point(contentPanel->Size.X, std::max(y + 4, contentPanel->Size.Y));
	if (highlightedY >= 0)
		contentPanel->SetScrollPosition(std::max(0, highlightedY - 20));
}

void PeriodicElementDetailActivity::OnDraw()
{
	auto *graphics = GetGraphics();
	graphics->DrawFilledRect(
		RectSized(Position - Vec2{ 1, 1 }, Size + Vec2{ 2, 2 }), 0x000000_rgb);
	graphics->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);
	graphics->BlendRect(
		RectSized(Position + contentPanel->Position - Vec2{ 1, 1 },
			contentPanel->Size + Vec2{ 2, 2 }),
		0xFFFFFF_rgb .WithAlpha(150));
}

void PeriodicElementDetailActivity::OnTick()
{
	if (exit)
		Exit();
}

void PeriodicElementDetailActivity::OnKeyPress(
	int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (!repeat && (key == SDLK_ESCAPE || key == SDLK_AC_BACK))
		exit = true;
}
