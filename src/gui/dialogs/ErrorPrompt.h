#ifndef ERRORPROMPT_H
#define ERRORPROMPT_H
#include <functional>
#include <string>
#include "interface/Window.h"

class ErrorPrompt : public ui::Window
{
	struct DismissCallback
	{
		std::function<void ()> dismiss;
	};
	DismissCallback callback;

public:
	ErrorPrompt(std::string message, std::string dismiss = "忽略");

	void SetCallback(DismissCallback callback) { this->callback = callback; }
	void OnExit(ui::DeleteReason deleteReason) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
