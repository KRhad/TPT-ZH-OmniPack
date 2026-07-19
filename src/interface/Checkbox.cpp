#include "Checkbox.h"
#include "Style.h"
#include "Window.h"
#include "common/tpt-minmax.h"
#include "game/ToolTip.h"
#include "graphics/VideoBuffer.h"

// TODO: Put checkbox in ui namespace and remove this
using namespace ui;

Checkbox::Checkbox(Point position, Point size_, std::string text_):
	Component(position, size_),
	checked(false),
	tooltip(NULL),
	textInside(false)
{
	if (size.X == AUTOSIZE || size.Y == AUTOSIZE)
	{
		Point checkboxSize = gfx::VideoBuffer::TextSize(text_);
		if (size.Y == AUTOSIZE)
			size.Y = 16;
		if (size.X == AUTOSIZE)
			size.X = checkboxSize.X + size.Y + 3;
	}
	if (size.X == TEXTINSIDE)
	{
		size.X = size.Y;
		textInside = true;
	}
	SetText(text_);
}

Checkbox::~Checkbox()
{
	delete tooltip;
}

void Checkbox::SetText(std::string text_)
{
	text = text_;
	// ensure text isn't too big for button, maybe not too efficient
	if (gfx::VideoBuffer::TextSize(text).X + (textInside ? 2 : size.Y+2) >= size.X)
	{
		text += "...";
		while (text.length() > 3 && gfx::VideoBuffer::TextSize(text).X + (textInside ? 2 : size.Y+2) >= size.X)
		{
			text.erase(text.length()-4, 1);
		}
	}
}

void Checkbox::SetTooltip(ToolTip *newTip)
{
	delete tooltip;
	tooltip = newTip;
}

void Checkbox::SetTooltipText(std::string newTooltip)
{
	if (tooltip)
		tooltip->SetTip(newTooltip);
}

void Checkbox::OnMouseUp(int x, int y, unsigned char button)
{
	if (IsClicked() && isMouseInside && enabled)
	{
		checked = !checked;
		if (callback)
		{
			callback(checked);
		}
	}
}

void Checkbox::OnDraw(gfx::VideoBuffer* vid)
{
	// clear area
	vid->FillRect(position.X, position.Y, size.X, size.Y, 0, 0, 0, 255);

	// border
	{
		pixel borderColor;
		if (IsClicked() && !checked)
			borderColor = COLADD(color, Style::ClickedModifier);
		else if (!enabled)
			borderColor = COLMULT(color, Style::DisabledMultiplier);
		else
			borderColor = color;
		vid->DrawRect(position.X, position.Y, size.Y, size.Y, borderColor);
	}

	ARGBColour textColor = color;
	ARGBColour innerColor = 0;
	if (!enabled)
	{
		textColor = COLMULT(color, Style::DisabledMultiplier);
	}
#ifdef TOUCHUI
	// Mouse not inside checkbox, Mouse not down, or over checkbox but click did not start on checkbox
	else if (!isMouseInside || !IsMouseDown() || (IsMouseDown() && !IsClicked()))
#else
	// Mouse not inside checkbox, or over checkbox but click did not start on checkbox
	else if (!isMouseInside || (IsMouseDown() && !IsClicked()))
#endif
	{
		if (checked)
		{
			innerColor = color;
		}
	}
	else
	{
		// button clicked and held down
		if (IsClicked())
		{
			if (!checked)
			{
				innerColor = color;
			}
		}
		// Mouse over button, not held down
		else
		{
			if (checked)
			{
				innerColor = COLMODALPHA(color, Style::InvertAlphaHover);
			}
			else
			{
				innerColor = COLMODALPHA(color, Style::HoverAlpha);
			}
		}
	}
	// inner checkbox color
	if (innerColor)
	{
		if (useCheckIcon)
			vid->DrawChar(position.X + size.Y / 2 - 3, position.Y + size.Y / 2 - 3, '\xCF', innerColor);
		else
			vid->FillRect(position.X+3, position.Y+3, size.Y-6, size.Y-6, innerColor);
	}

	Point textSize = gfx::VideoBuffer::TextSize(text);
	int textPosX = textInside ? position.X + (size.X - textSize.X) / 2 : position.X + size.Y + 2;
	int textPosY = position.Y + (size.Y - textSize.Y + 1) / 2 + 1;
	vid->DrawString(textPosX, textPosY, text, textColor);
}

void Checkbox::OnTick(uint32_t ticks)
{
#ifdef TOUCHUI
	if (isMouseInside && enabled && IsMouseDown() && tooltip)
#else
	if (isMouseInside && enabled && tooltip)
#endif
		tooltip->AddToScreen();
}
