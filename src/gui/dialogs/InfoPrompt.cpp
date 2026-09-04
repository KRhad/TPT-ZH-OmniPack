#include "InfoPrompt.h"
#include "defines.h"
#include "common/Point.h"
#include "interface/Label.h"
#include "interface/Button.h"
#include "interface/ScrollWindow.h"

InfoPrompt::InfoPrompt(std::string title, std::string message, std::string OK, bool large):
	ui::Window(Point(CENTERED, CENTERED), Point(250, 55))
{
#ifndef TOUCHUI
	int buttonHeight = 15;
#else
	int buttonHeight = 25;
#endif

	Label *titleLabel = new Label(Point(5, 3), Point(Label::AUTOSIZE, Label::AUTOSIZE), title);
	titleLabel->SetColor(COLRGB(140, 140, 255));
	this->AddComponent(titleLabel);

	if (large)
	{
		this->Resize(Point(CENTERED, CENTERED), Point(550, YRES + MENUSIZE - 16));
		int headerHeight = titleLabel->GetPosition().Y + titleLabel->GetSize().Y;
		auto *scrollArea = new ui::ScrollWindow(Point(0, headerHeight), this->size - Point(0, headerHeight + buttonHeight));
		auto *messageLabel = new Label(Point(5, 3), Point(scrollArea->GetUsableWidth() - 10, Label::AUTOSIZE), message, true);
		messageLabel->SetSelectable(false);
		scrollArea->AddComponent(messageLabel);
		scrollArea->SetScrollSize(messageLabel->GetSize().Y + 6);
		this->AddSubwindow(scrollArea);
	}
	else
	{
		Label *messageLabel = new Label(titleLabel->Below(Point(0, 0)), Point(240, Label::AUTOSIZE), message, true);
		this->Resize(Point(CENTERED, CENTERED), Point(250, messageLabel->GetSize().Y + 24 + buttonHeight));
		this->AddComponent(messageLabel);
	}

	Button *okButton = new Button(Point(0, this->size.Y - buttonHeight), Point(this->size.X, buttonHeight), OK);
	okButton->SetTextColor(COLRGB(140, 140, 255));
	okButton->SetCloseButton(true);
	this->AddComponent(okButton);
}

void InfoPrompt::OnExit(ui::DeleteReason deleteReason)
{
	if (callback.dismiss)
		callback.dismiss();
}

void InfoPrompt::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (key == SDLK_RETURN)
		this->Close(ui::Confirmed);
}
