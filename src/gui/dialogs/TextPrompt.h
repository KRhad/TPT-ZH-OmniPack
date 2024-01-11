#ifndef TEXTPROMPT_H
#define TEXTPROMPT_H
#include <functional>
#include <optional>
#include <string>
#include "interface/Window.h"

class Textbox;
class TextPrompt : public ui::Window
{
	struct InputCallback
	{
		std::function<void (std::optional<std::string>)> input;
	};
	InputCallback callback;

	Textbox *inputTextbox;
public:
	TextPrompt(std::string title, std::string prompt, std::string text, std::string shadow);

	void SetCallback(InputCallback callback) { this->callback = callback; }

	void OnExit(DeleteReason deleteReason) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
