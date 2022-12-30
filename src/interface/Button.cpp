#include "Button.h"
#include "Style.h"
#include "Window.h"
#include "common/tpt-minmax.h"
#include "common/Platform.h"
#include "game/ToolTip.h"
#include "graphics/VideoBuffer.h"

// TODO: Put button in ui namespace and remove this
using namespace ui;

Button::Button(Point position, Point size_, std::string text_):
	Component(position, size_),
	textColor(Style::Border),
	backgroundColor(0),
	tooltip(NULL),
	alignment(CENTER),
	state(NORMAL),
	isCloseButton(false),
	timeHeldDown(0),
	didVibrate(false)
{
	if (size.X == AUTOSIZE || size.Y == AUTOSIZE)
	{
		Point buttonSize = gfx::VideoBuffer::TextSize(text_);
		if (size.X == AUTOSIZE)
			size.X = buttonSize.X + 7;
		if (size.Y == AUTOSIZE)
			size.Y = buttonSize.Y + 7;
	}
	SetText(text_);
}

Button::~Button()
{
	delete tooltip;
}

void Button::SetText(std::string text_)
{
	text = text_;
	// ensure text isn't too big for button, maybe not too efficient
	if (gfx::VideoBuffer::TextSize(text).X+2 >= size.X)
	{
		text += "...";
		while (text.length() > 3 && gfx::VideoBuffer::TextSize(text).X+2 >= size.X)
		{
			text.erase(text.length()-4, 1);
		}
	}
}

void Button::SetColor(ARGBColour newColor)
{
	color = newColor;
	textColor = newColor;
}

void Button::SetTextColor(ARGBColour newColor)
{
	textColor = newColor;
}

void Button::SetBackgroundColor(ARGBColour newColor)
{
	backgroundColor = newColor;
}

void Button::SetTooltip(ToolTip *newTip)
{
	delete tooltip;
	tooltip = newTip;
}

void Button::SetTooltipText(std::string newTooltip)
{
	if (tooltip)
		tooltip->SetTip(newTooltip);
}

void Button::OnMouseDown(int x, int y, unsigned char button)
{

}

void Button::OnMouseUp(int x, int y, unsigned char button)
{
	if (IsClicked() && isMouseInside && enabled)
	{
		if (callback)
		{
			if (state == HOLD)
				callback(IsHeld() ? 3 : button);
			else
				callback(button);
		}
		if (isCloseButton && GetParent())
			GetParent()->toDelete = true;
	}
}

void Button::OnDraw(gfx::VideoBuffer* vid)
{
	// clear area
	vid->FillRect(position.X, position.Y, size.X, size.Y, 0, 0, 0, 255);

	// border
	if (enabled)
		vid->DrawRect(position.X, position.Y, size.X, size.Y, color);
	else
		vid->DrawRect(position.X, position.Y, size.X, size.Y, COLMULT(color, Style::DisabledMultiplier));

	ARGBColour realTextColor;
	ARGBColour backColor = 0;
	if (!enabled)
	{
		if (state == INVERTED)
			backColor = color;
		else if (backgroundColor)
			backColor = backgroundColor;
		realTextColor = COLMULT(textColor, Style::DisabledMultiplier);
	}
#ifdef TOUCHUI
	// Mouse not inside button, Mouse not down, or over button but click did not start on button
	else if (!isMouseInside || !IsMouseDown() || (IsMouseDown() && !IsClicked()))
#else
	// Mouse not inside button, or over button but click did not start on button
	else if (!isMouseInside || (IsMouseDown() && !IsClicked()))
#endif
	{
		if (state == INVERTED)
		{
			realTextColor = COLRGB(255 - COLR(textColor), 255 - COLG(textColor), 255 - COLB(textColor));
			backColor = color;
		}
		else
		{
			backColor = backgroundColor;
			realTextColor = textColor;
		}
	}
	else
	{
		// button clicked and held down
		if (IsClicked())
		{
			if (state == INVERTED)
				realTextColor = textColor;
			else if (state == HOLD)
			{
				if (IsHeld())
				{
					realTextColor = COLPACK(0x000000);
					backColor = color;
				}
				else
				{
					unsigned int heldAmount = std::min((int)(timeHeldDown / 20), 100);
					realTextColor = textColor;
					backColor = COLMODALPHA(color, 100 + heldAmount);
				}
			}
			else if (backgroundColor)
			{
				backColor = COLMODALPHA(color, Style::HighlightAlphaHover);;
				realTextColor = COLPACK(0x000000);
			}
			else
			{
				backColor = color;
				realTextColor = COLPACK(0x000000);
			}
		}
		// Mouse over button, not held down
		else
		{
			if (state == INVERTED)
			{
				backColor = COLMODALPHA(color, Style::InvertAlphaHover);
				realTextColor = COLRGB(255 - COLR(textColor), 255 - COLG(textColor), 255 - COLB(textColor));
			}
			else
			{
				backColor = COLMODALPHA(color, Style::HoverAlpha);
				realTextColor = textColor;
			}
		}
	}
	if (backColor)
		vid->FillRect(position.X+1, position.Y+1, size.X-2, size.Y-2, backColor);

	if (alignment == LEFT)
	{
		Point textSize = gfx::VideoBuffer::TextSize(text);
		vid->DrawString(position.X+2, position.Y+(size.Y-textSize.Y+1)/2+1, text, realTextColor);
	}
	else
	{
		Point textSize = gfx::VideoBuffer::TextSize(text);
		vid->DrawString(position.X+(size.X-textSize.X)/2+1, position.Y+(size.Y-textSize.Y+1)/2+1, text, realTextColor);
	}
}

void Button::OnTick(uint32_t ticks)
{
#ifdef TOUCHUI
	if (isMouseInside && enabled && IsMouseDown() && tooltip)
#else
	if (isMouseInside && enabled && tooltip)
#endif
		tooltip->AddToScreen();

	if (state == HOLD)
	{
		if (IsClicked())
			timeHeldDown += ticks;
		else
		{
			timeHeldDown = 0;
			didVibrate = false;
		}

		if (!didVibrate && timeHeldDown > heldThreshold)
		{
			Platform::Vibrate(10);
			didVibrate = true;
		}
	}
}
