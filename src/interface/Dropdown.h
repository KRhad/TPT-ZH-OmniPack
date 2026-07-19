#ifndef DROPDOWN_H
#define DROPDOWN_H

#include <functional>
#include <string>
#include <vector>
#include "Component.h"
#include "common/Point.h"
#include "interface/Window.h"

namespace
{
class VideoBuffer;
}

class Dropdown : public Component
{
	std::vector<std::string> options;
	unsigned int selectedOption = 0;
	bool isSelectingOption = false;
	std::function<void(unsigned int)> callback = nullptr;

public:
	Dropdown(Point position, Point size, std::vector<std::string> options);

	void SetCallback(std::function<void(unsigned int)> const & callback) { this->callback = callback; }
	unsigned int GetSelectedOption() { return selectedOption; }
	void SetSelectedOption(unsigned int selectedOption);
	bool IsSelectingOption() { return isSelectingOption; }

	void OnMouseUp(int x, int y, unsigned char button) override;
	void OnDraw(gfx::VideoBuffer* vid) override;

	static const int AUTOSIZE = -1;

	friend class DropdownOptions;
};

class DropdownOptions : public ui::Window
{
	Dropdown * dropdown;
	unsigned int hoveredOption = -1;
	unsigned int optionHeight;

public:
	DropdownOptions(Point position, Point size, Dropdown * dropdown);
	~DropdownOptions() override;

	void OnMouseMove(int x, int y, Point difference) override;
	void OnMouseUp(int x, int y, unsigned char button) override;
	void OnDraw(gfx::VideoBuffer* vid) override;
};

#endif
