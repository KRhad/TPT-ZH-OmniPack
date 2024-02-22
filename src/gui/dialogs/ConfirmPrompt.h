#ifndef COMFIRMPROMPT_H
#define COMFIRMPROMPT_H

#include <functional>
#include <string>
#include "interface/Window.h"

class ConfirmPrompt : public ui::Window
{
	struct ConfirmCallback
	{
		std::function<void(bool)> confirm;
	};
	ConfirmCallback callback;

public:
	ConfirmPrompt(std::string title, std::string message, std::string OK = "OK", std::string cancel = "Cancel");

	void SetCallback(ConfirmCallback callback) { this->callback = callback; }

	void OnExit(ui::DeleteReason deleteReason) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
