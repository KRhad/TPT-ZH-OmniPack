#include "Dropdown.h"
#include "Style.h"
#include "common/tpt-minmax.h"
#include "graphics/VideoBuffer.h"
#include "interface/Engine.h"
#include "interface/Window.h"

// TODO: Put dropdown in ui namespace and remove this
using namespace ui;

Dropdown::Dropdown(Point position, Point size, std::vector<std::string> options):
	Component(position, size),
	options(options)
{
	if (size.X == AUTOSIZE)
	{
		int maxWidth = 25;
		for (const auto &option : options)
			maxWidth = tpt::max(maxWidth, gfx::VideoBuffer::TextSize(option).X);
		this->size.X = maxWidth + 5;
	}
	if (size.Y == AUTOSIZE)
	{
#ifndef TOUCHUI
		this->size.Y = 16;
#else
		this->size.Y = 24;
#endif
	}
}

void Dropdown::SetSelectedOption(unsigned int selectedOption)
{
	if (selectedOption < options.size())
		this->selectedOption = selectedOption;
}

void Dropdown::OnMouseUp(int x, int y, unsigned char button)
{
	if (IsClicked())
	{
		Point windowPos = position;
		Point windowSize = size;
		windowPos.X -= 2;
		windowSize.X += 4;

		int numOptions = options.size();
		int optionHeight = windowSize.Y;
		windowPos.Y = windowPos.Y - ((numOptions - 1) * optionHeight) / 2 + 1;
		windowSize.Y = numOptions * (optionHeight - 1) + 1;

		auto *dropdownOptions = new DropdownOptions(windowPos, windowSize, this);

		// Detect if this dropdown box will fit inside the parent window
		// If not, make it a standalone window instead of a subwindow
		Point parentTopLeft = this->GetParent()->GetPosition();
		Point parentBottomRight = parentTopLeft + this->GetParent()->GetSize();
		if (!windowPos.IsInside(parentTopLeft, parentBottomRight)
			&& !(windowPos + windowSize).IsInside(parentTopLeft, parentBottomRight))
		{
			dropdownOptions->SetPosition(windowPos + parentTopLeft);
			Engine::Ref().ShowWindow(dropdownOptions);
		}
		else
		{
			this->GetParent()->AddSubwindow(dropdownOptions);
		}

		isSelectingOption = true;
	}
}

void Dropdown::OnDraw(gfx::VideoBuffer* vid)
{
	ARGBColour col;
	if (!enabled)
		col = COLMULT(color, Style::DisabledMultiplier);
	else if (!isMouseInside)
		col = COLMULT(color, Style::DeselectedMultiplier);
	else if (IsClicked())
		col = COLADD(color, Style::ClickedModifier);
	else
		col = color;

	vid->DrawRect(position.X, position.Y, size.X, size.Y, col);
	if (selectedOption < options.size())
		vid->DrawString(position.X + 3, position.Y + (size.Y / 2) - 4, options[selectedOption], color);
}


DropdownOptions::DropdownOptions(Point position, Point size, Dropdown * dropdown):
	ui::Window(position, size),
	dropdown(dropdown)
{
	optionHeight = dropdown->size.Y - 1;
}

DropdownOptions::~DropdownOptions()
{
	dropdown->isSelectingOption = false;
}

void DropdownOptions::OnMouseMove(int x, int y, Point difference)
{
#ifndef TOUCHUI
	if (Point(x, y).IsInside(position, position + size))
		hoveredOption = tpt::min((size_t)((y - position.Y) / optionHeight), dropdown->options.size() - 1);
	else
		hoveredOption = -1;
#endif
}

void DropdownOptions::OnMouseUp(int x, int y, unsigned char button)
{
	if (Point(x, y).IsInside(position, position + size))
	{
		dropdown->selectedOption = (y - position.Y) / optionHeight;
		if (dropdown->callback)
			dropdown->callback(dropdown->selectedOption);
	}
	Close(Confirmed);
}

void DropdownOptions::OnDraw(gfx::VideoBuffer* vid)
{
	int optionHeight = dropdown->size.Y;
	for (size_t i = 0; i < dropdown->options.size(); i++)
	{
		int yPos = (i * (optionHeight-1));
		if (i == dropdown->selectedOption)
			vid->FillRect(0, yPos, size.X, optionHeight, 75, 75, 75, 255);
		else if (i == hoveredOption)
			vid->FillRect(0, yPos, size.X, optionHeight, 50, 50, 50, 255);
		else
			vid->FillRect(0, yPos, size.X, optionHeight, 0, 0, 0, 255);
		vid->DrawRect(0, yPos, size.X, optionHeight, 255, 255, 255, 255);
		vid->DrawString(3, yPos + (optionHeight / 2) - 4, dropdown->options[i], 255, 255, 255, 255);

	}
}
