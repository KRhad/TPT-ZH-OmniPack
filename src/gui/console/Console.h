#ifndef CONSOLE_H
#define CONSOLE_H

#include <deque>
#include <string>
#include "defines.h"
#include "interface/Window.h"

class Button;
class Label;
class Textbox;
namespace ui
{
	class ScrollWindow;
}
class Console : public ui::Window
{
	ui::ScrollWindow *scrollWindow;
	std::deque<std::pair<Label*, Label*>> consoleHistoryLabels;
	Textbox *commandTextbox;
	Button *submitButton;
	int dividerPos = VIDXRES / 2 - 50;
	bool draggingDivider = false;
	unsigned char lastClickButton = 0;
	int historyLoc = -1;

	void InitHistoryLabels();
	void ResizeHistoryLabels();
	void Rerun(std::string command);
	void CheckSubmitButton();
	void Submit(std::string command);
	std::string RunCommand(std::string command);
	std::pair<Label*, Label*> GetHistoryLabels(std::string command, std::string result);
	int AddHistoryLabels(Label *commandLabel, Label *resultLabel, int lowerYPos, bool adjustForLabelHeight);
	int GetHistoryItemHeight(std::pair<Label*, Label*> labels);
	void RepositionLabels(int adjustAmount);
	void DiscardOldestCommand();
	void SetHistoryLoc(int historyLoc);

	void OnSubwindowDraw(gfx::VideoBuffer *buf);

public:
	Console();
	~Console();

	void OnTick(uint32_t ticks) override;
	void OnDraw(gfx::VideoBuffer *buf) override;
	void OnMouseMove(int x, int y, Point difference) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;

	bool BeforeMouseDown(int x, int y, unsigned char button) override;
	bool BeforeMouseUp(int x, int y, unsigned char button) override;
	bool BeforeTextInput(const char *text) override;
};

extern std::deque<std::pair<std::string, std::string>> consoleHistory;
extern std::string unsubmittedCommand;

#endif // CONSOLE_H
