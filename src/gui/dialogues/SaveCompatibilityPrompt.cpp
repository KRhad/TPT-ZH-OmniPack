#include "SaveCompatibilityPrompt.h"

#include "common/Localization.h"
#include "gui/Style.h"
#include "gui/interface/Button.h"
#include "gui/interface/Engine.h"
#include "gui/interface/Label.h"
#include "gui/interface/ScrollPanel.h"
#include "graphics/Graphics.h"

#include <utility>

SaveCompatibilityPrompt::SaveCompatibilityPrompt(String title, String message, ResultCallback callback_):
	ui::Window(ui::Point(-1, -1), ui::Point(340, 50)),
	callback(std::move(callback_))
{
	ui::Label *titleLabel = new ui::Label(ui::Point(4, 5), ui::Point(Size.X - 8, 15), title);
	titleLabel->SetTextColour(style::Colour::WarningTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);

	ui::ScrollPanel *messagePanel = new ui::ScrollPanel(ui::Point(4, 24), ui::Point(Size.X - 8, 206));
	AddComponent(messagePanel);

	ui::Label *messageLabel = new ui::Label(ui::Point(4, 0), ui::Point(Size.X - 28, -1), message);
	messageLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	messageLabel->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	messageLabel->SetMultiline(true);
	messagePanel->AddChild(messageLabel);
	messagePanel->InnerSize = ui::Point(messagePanel->Size.X, messageLabel->Size.Y + 4);

	if (messageLabel->Size.Y < messagePanel->Size.Y)
	{
		messagePanel->Size.Y = messageLabel->Size.Y + 4;
	}
	Size.Y += messagePanel->Size.Y + 12;
	Position.Y = (GetGraphics()->Size().Y - Size.Y) / 2;

	constexpr int buttonWidth = 112;
	ui::Button *cancelButton = new ui::Button(ui::Point(0, Size.Y - 16), ui::Point(buttonWidth, 16), Localization::Ref().Tr("dialog.cancel"));
	cancelButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	cancelButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	cancelButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	cancelButton->SetActionCallback({ [this] {
		CloseActiveWindow();
		SelfDestruct();
	} });
	AddComponent(cancelButton);
	SetCancelButton(cancelButton);

	ui::Button *readOnlyButton = new ui::Button(ui::Point(buttonWidth + 2, Size.Y - 16), ui::Point(buttonWidth, 16), Localization::Ref().Tr("gamecontroller.load_read_only"));
	readOnlyButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	readOnlyButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	readOnlyButton->SetActionCallback({ [this] {
		CloseActiveWindow();
		if (callback.loadReadOnly)
		{
			callback.loadReadOnly();
		}
		SelfDestruct();
	} });
	AddComponent(readOnlyButton);

	ui::Button *normalButton = new ui::Button(ui::Point(buttonWidth * 2 + 4, Size.Y - 16), ui::Point(buttonWidth, 16), Localization::Ref().Tr("gamecontroller.load_normally"));
	normalButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	normalButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	normalButton->Appearance.TextInactive = style::Colour::WarningTitle;
	normalButton->SetActionCallback({ [this] {
		CloseActiveWindow();
		if (callback.loadNormally)
		{
			callback.loadNormally();
		}
		SelfDestruct();
	} });
	AddComponent(normalButton);
	SetOkayButton(normalButton);

	MakeActiveWindow();
}

void SaveCompatibilityPrompt::OnDraw()
{
	Graphics *g = GetGraphics();
	g->DrawFilledRect(RectSized(Position - Vec2{ 1, 1 }, Size + Vec2{ 2, 2 }), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xC8C8C8_rgb);
}
