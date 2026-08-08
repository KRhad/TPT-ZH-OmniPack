#pragma once

#include "Activity.h"
#include "simulation/PeriodicTableData.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class GameController;
class Tool;

namespace ui
{
	class ScrollPanel;
}

class PeriodicElementDetailActivity: public WindowActivity
{
	GameController *gameController;
	std::vector<Tool *> tools;
	PeriodicElementRecord element;
	std::string highlightedIdentifier;
	std::function<void(Tool *)> selectedCallback;
	ui::ScrollPanel *contentPanel = nullptr;
	bool exit = false;

	Tool *FindTool(std::string_view identifier) const;
	void BuildContent();

public:
	PeriodicElementDetailActivity(
		GameController *gameController,
		std::vector<Tool *> tools,
		PeriodicElementRecord const &element,
		std::string highlightedIdentifier,
		std::function<void(Tool *)> selectedCallback);
	~PeriodicElementDetailActivity() override;

	void OnDraw() override;
	void OnTick() override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};
