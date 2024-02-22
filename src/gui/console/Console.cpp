#include "Console.h"
#include "legacy_console.h"
#include "luaconsole.h"
#include "misc.h"
#include "common/tpt-minmax.h"
#include "graphics/ARGBColour.h"
#include "graphics/VideoBuffer.h"
#include "interface/Button.h"
#include "interface/Engine.h"
#include "interface/Label.h"
#include "interface/ScrollWindow.h"
#include "interface/Style.h"
#include "interface/Textbox.h"

std::deque<std::pair<std::string, std::string>> consoleHistory;
std::string unsubmittedCommand;

Console::Console():
	ui::Window(Point(0, 0), Point(VIDXRES, 250))
{
	HasBorder(false);
	SetTransparency(60);

	scrollWindow = new ui::ScrollWindow(Point(0, 0), Point(VIDXRES, size.Y - 17));
	scrollWindow->SetOnDraw([this](gfx::VideoBuffer *buf) { OnSubwindowDraw(buf); });
	this->AddSubwindow(scrollWindow);

	commandTextbox = new Textbox(Point(16, size.Y - 17), Point(VIDXRES - 16, 17), "", true);
	commandTextbox->SetAutoSize(false, true, Point(Textbox::NOSIZELIMIT, Textbox::NOSIZELIMIT));
	commandTextbox->SetCallback([&]() { this->CheckSubmitButton(); });
	if (!unsubmittedCommand.empty())
		commandTextbox->SetText(unsubmittedCommand);
	this->AddComponent(commandTextbox);
#ifndef TOUCHUI
	FocusComponent(commandTextbox);
#endif

	submitButton = new Button(Point(0, size.Y - 17), Point(17, 17), "\xCB");
	submitButton->SetTextColor(COLARGB(255, 255, 0, 0));
	CheckSubmitButton();
	submitButton->SetCallback([&](int button)
	{
		if (!commandTextbox->GetText().empty())
		{
			this->Submit(commandTextbox->GetText());
			commandTextbox->SetText("");
		}
	});
	this->AddComponent(submitButton);

	InitHistoryLabels();
	console_mode = true; // legacy flag, but still used for several things
}

Console::~Console()
{
	console_mode = false; // legacy flag, but still used for several things
	if (historyLoc == -1)
		unsubmittedCommand = commandTextbox->GetText();
	else
		unsubmittedCommand = "";
}

void Console::InitHistoryLabels()
{
	int yPos = scrollWindow->GetSize().Y;
	for (auto &history : consoleHistory)
	{
		std::pair<Label*, Label*> labels = GetHistoryLabels(history.first, history.second);
		yPos -= AddHistoryLabels(labels.first, labels.second, yPos, true);
		consoleHistoryLabels.push_back(labels);
	}
	// Allow some extra room up top
	yPos -= 10;

	scrollWindow->SetScrollSize(scrollWindow->GetSize().Y + -yPos, true);
	if (yPos < 0)
		RepositionLabels(scrollWindow->GetScrollSize() - scrollWindow->GetSize().Y);
}

void Console::ResizeHistoryLabels()
{
	scrollWindow->SetScrollPosition(0);
	int yPos = scrollWindow->GetSize().Y;
	for (auto &labels : consoleHistoryLabels)
	{
		//std::pair<Label*, Label*> labels = GetHistoryLabels(consoleHistory.first, consoleHistory.second);
		labels.first->SetSize(Point(dividerPos - 21, Label::AUTOSIZE));
		labels.second->SetSize(Point(scrollWindow->GetUsableWidth() - dividerPos - 15, Label::AUTOSIZE));
		yPos -= AddHistoryLabels(labels.first, labels.second, yPos, true);
	}
	// Allow some extra room up top
	yPos -= 10;

	scrollWindow->SetScrollSize(scrollWindow->GetSize().Y + -yPos, true);
	if (yPos < 0)
		RepositionLabels(scrollWindow->GetScrollSize() - scrollWindow->GetSize().Y);
	scrollWindow->SetScrollPosition(scrollWindow->GetScrollSize());
}

void Console::Rerun(std::string command)
{
	commandTextbox->SetText(command);
	CheckSubmitButton();
}

void Console::CheckSubmitButton()
{
	submitButton->SetEnabled(!commandTextbox->GetText().empty());
}

void Console::Submit(std::string command)
{
	std::string result = RunCommand(command);

	std::pair<Label*, Label*> labels = GetHistoryLabels(command, result);
	int lowerYPos = scrollWindow->GetScrollSize() - scrollWindow->GetScrollPosition();
	if (!scrollWindow->IsScrollable())
		lowerYPos = scrollWindow->GetSize().Y;
	int labelHeight = AddHistoryLabels(labels.first, labels.second, lowerYPos, false);

	consoleHistoryLabels.push_front(labels);
	consoleHistory.push_front({command, result});
	DiscardOldestCommand();
	// Add height of new label to the window's scroll size
	scrollWindow->SetScrollSize(scrollWindow->GetScrollSize() + labelHeight, true);
	// Not currently scrollable, so we need to move all labels upwards ourselves (ScrollWindow not handling this yet)
	if (!scrollWindow->IsScrollable())
		RepositionLabels(-labelHeight);
	unsubmittedCommand = "";
	historyLoc = -1;
	save_presets();
}

std::string Console::RunCommand(std::string command)
{
	std::string result;
	int ret;
#ifdef LUACONSOLE
	ret = process_command_lua(vid_buf, command.c_str(), &result);
#else
	ret = process_command_old(globalSim, vid_buf, command, &result);
#endif
	if (ret == -1)
	{
		this->Close(ui::Confirmed);
	}
	else if (ret == -2)
	{
		Engine::Ref().Shutdown();
	}
	return result;
}

std::pair<Label*, Label*> Console::GetHistoryLabels(std::string command, std::string result)
{
	Label *commandLabel = new Label(Point(16, 0), Point(dividerPos - 21, Label::AUTOSIZE), command, true);
	Label *resultLabel = new Label(Point(0, 0), Point(scrollWindow->GetUsableWidth() - dividerPos - 15, Label::AUTOSIZE), result, true);
	return {commandLabel, resultLabel};
}

int Console::AddHistoryLabels(Label *commandLabel, Label *resultLabel, int lowerYPos, bool adjustForLabelHeight)
{
	int sizeY = tpt::max<int>(commandLabel->GetSize().Y, resultLabel->GetSize().Y);
	int posY = lowerYPos - (adjustForLabelHeight ? sizeY : 0);
	commandLabel->SetPosition(Point(16, posY));
	resultLabel->SetPosition(Point(dividerPos + 10, posY));
	scrollWindow->AddComponent(commandLabel);
	scrollWindow->AddComponent(resultLabel);

	return sizeY;
}

int Console::GetHistoryItemHeight(std::pair<Label*, Label*> labels)
{
	return tpt::max<int>(labels.first->GetSize().Y, labels.second->GetSize().Y);
}

void Console::RepositionLabels(int adjustAmount)
{
	for (size_t i = 0; i < consoleHistoryLabels.size(); i++)
	{
		auto historyLabels = consoleHistoryLabels.at(i);
		historyLabels.first->SetPosition(historyLabels.first->GetPosition() - Point(0, -adjustAmount));
		historyLabels.second->SetPosition(historyLabels.second->GetPosition() - Point(0, -adjustAmount));
	}
}

void Console::DiscardOldestCommand()
{
	int subtractedSize = 0;
	while (consoleHistoryLabels.size() > 20)
	{
		auto consoleHistory = consoleHistoryLabels.end() - 1;
		scrollWindow->RemoveComponent(consoleHistory->first);
		scrollWindow->RemoveComponent(consoleHistory->second);
		consoleHistoryLabels.pop_back();
		subtractedSize += tpt::max(consoleHistory->first->GetSize().Y, consoleHistory->second->GetSize().Y);
	}
	while (consoleHistory.size() > 20)
		consoleHistory.pop_back();

	int oldScrollSize = scrollWindow->GetScrollSize();
	scrollWindow->SetScrollSize(scrollWindow->GetScrollSize() - subtractedSize, true);
	if (scrollWindow->GetScrollSize() > 0)
		RepositionLabels(-tpt::min(subtractedSize, oldScrollSize));
}

void Console::SetHistoryLoc(int historyLoc)
{
	if (this->historyLoc == -1)
		unsubmittedCommand = commandTextbox->GetText();
	this->historyLoc = historyLoc;

	if (historyLoc >= 0 && historyLoc < static_cast<int>(consoleHistory.size()))
		commandTextbox->SetText(consoleHistory.at(historyLoc).first);
	else if (historyLoc == -1)
		commandTextbox->SetText(unsubmittedCommand);
	else
		commandTextbox->SetText("");
	CheckSubmitButton();
}

void Console::OnSubwindowDraw(gfx::VideoBuffer *buf)
{
	ARGBColour lineColor;
	if (draggingDivider)
		lineColor = ui::Style::Border;
	else
		lineColor = COLMULT(ui::Style::Border, ui::Style::DeselectedMultiplier);
	buf->DrawLine(dividerPos, 0, dividerPos, size.Y, lineColor);
}

void Console::OnTick(uint32_t ticks)
{
	if (scrollWindow->GetSize().Y + commandTextbox->GetSize().Y != size.Y)
	{
		this->Resize(position, Point(size.X, scrollWindow->GetSize().Y + commandTextbox->GetSize().Y));
	}
	// If labels are right clicked (or clicked at all on android), then replace the textbox text with this
	for (auto &label : consoleHistoryLabels)
	{
#ifndef TOUCHUI
		if (scrollWindow->IsClicked(label.first) && lastClickButton == 3)
#else
		if (scrollWindow->IsClicked(label.first))
#endif
		{
			this->Rerun(label.first->GetText());
		}
	}
}

void Console::OnDraw(gfx::VideoBuffer *buf)
{
	buf->DrawLine(0, size.Y - 1, VIDXRES, size.Y - 1, ui::Style::Border);
}

void Console::OnMouseMove(int x, int y, Point difference)
{
	if (draggingDivider)
	{
		dividerPos = tpt::max(100, tpt::min(x, size.X - 100));
		ResizeHistoryLabels();
	}
}

bool Console::BeforeMouseDown(int x, int y, unsigned char button)
{
	if (button == 1 && x > dividerPos - 8 && x < dividerPos + 8 && y < scrollWindow->GetSize().Y)
		draggingDivider = true;
	lastClickButton = button;
	return true;
}

bool Console::BeforeMouseUp(int x, int y, unsigned char button)
{
	draggingDivider = false;
	return true;
}

void Console::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (key == SDLK_RETURN)
	{
		if (!commandTextbox->GetText().empty())
		{
			Submit(commandTextbox->GetText());
			commandTextbox->SetText("");
		}
	}
	else if (graveExitsConsole && !shift && scan == SDL_SCANCODE_GRAVE)
	{
		this->Close(ui::Escape);
	}
	else if (key == SDLK_UP)
	{
		if (historyLoc < static_cast<int>(consoleHistory.size()) - 1)
			SetHistoryLoc(historyLoc + 1);
	}
	else if (key == SDLK_DOWN)
	{
		if (historyLoc >= static_cast<int>(unsubmittedCommand.empty() ? 0 : -1))
			SetHistoryLoc(historyLoc - 1);
	}
}

bool Console::BeforeTextInput(const char *text)
{
	if (!strcmp(text, "`"))
		return false;
	return true;
}
