#pragma once

#include "gui/interface/Window.h"

#include <functional>

class SaveCompatibilityPrompt : public ui::Window
{
	struct ResultCallback
	{
		std::function<void ()> loadNormally;
		std::function<void ()> loadReadOnly;
	};

	ResultCallback callback;

public:
	SaveCompatibilityPrompt(String title, String message, ResultCallback callback_ = {});
	~SaveCompatibilityPrompt() override = default;

	void OnDraw() override;
};
