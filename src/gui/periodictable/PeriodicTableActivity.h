#pragma once

#include "Activity.h"
#include "common/String.h"

#include <string_view>
#include <vector>

class GameController;
class Tool;

namespace ui
{
	class Button;
	class Label;
	class Textbox;
}

class PeriodicTableActivity: public WindowActivity
{
	GameController *gameController;
	std::vector<Tool *> tools;
	ui::Textbox *searchField = nullptr;
	ui::Button *stateFilterButton = nullptr;
	ui::Button *classFilterButton = nullptr;
	ui::Button *radioactivityFilterButton = nullptr;
	ui::Button *seriesButton = nullptr;
	ui::Label *statusLabel = nullptr;
	std::vector<ui::Button *> elementButtons;
	int stateFilter = 0;
	int classFilter = 0;
	int radioactivityFilter = 0;
	bool showSeries = true;
	bool exit = false;

	void RebuildElements();
	void RefreshFilterLabels();
	Tool *FindTool(std::string_view identifier) const;

public:
	PeriodicTableActivity(GameController *gameController, std::vector<Tool *> tools);
	~PeriodicTableActivity() override;

	void OnDraw() override;
	void OnTick() override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};
