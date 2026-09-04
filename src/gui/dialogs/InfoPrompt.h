#ifndef INFOPROMPT_H
#define INFOPROMPT_H
#include <functional>
#include <string>
#include "interface/Window.h"

class InfoPrompt : public ui::Window
{
	struct DismissCallback
	{
		std::function<void ()> dismiss;
	};
	DismissCallback callback;

public:
	InfoPrompt(std::string title, std::string message, std::string OK = "确定", bool large = false);

	void SetCallback(DismissCallback callback) { this->callback = callback; }

	void OnExit(ui::DeleteReason deleteReason) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
