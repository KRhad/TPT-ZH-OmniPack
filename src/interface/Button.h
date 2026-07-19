#ifndef BUTTON_H
#define BUTTON_H

#include <functional>
#include <string>
#include "common/Point.h"
#include "graphics/ARGBColour.h"
#include "Component.h"

namespace gfx
{
class VideoBuffer;
}
class ToolTip;

class Button : public Component
{
public:
	enum TextAlign { LEFT, CENTER };
	enum State { NORMAL, INVERTED, HOLD };
	static const uint32_t heldThreshold = 1000;

private:
	std::string text;
	ARGBColour textColor;
	ARGBColour backgroundColor;
	ToolTip *tooltip;
	std::function<void(int)> callback = nullptr;
	TextAlign alignment;
	State state;
	bool isCloseButton, isConfirmButton;

	// "hold" button does different actions when you hold it for one second
	uint32_t timeHeldDown;
	bool didVibrate;

public:
	Button(Point position, Point size, std::string text_);
	virtual ~Button();

	std::string GetText() { return text; }
	void SetText(std::string text_);
	void SetColor(ARGBColour newColor) override;
	void SetTextColor(ARGBColour newColor);
	void SetBackgroundColor(ARGBColour newColor);
	void SetTooltip(ToolTip *newTip);
	void SetTooltipText(std::string newTooltip);
	void SetCallback(std::function<void(int)> callback) { this->callback = callback; }
	void SetAlign(TextAlign align) { alignment = align; }
	void SetState(State state_) { state = state_; }
	void SetCloseButton(bool isCloseButton) { this->isCloseButton = isCloseButton; }
	void SetConfirmButton(bool isConfirmButton) { this->isConfirmButton = isConfirmButton; }

	bool IsHeld() { return timeHeldDown > heldThreshold; }

	void OnMouseDown(int x, int y, unsigned char button) override;
	void OnMouseUp(int x, int y, unsigned char button) override;
	void OnDraw(gfx::VideoBuffer* vid) override;
	void OnTick(uint32_t ticks) override;

	static const int AUTOSIZE = -1;
};

#endif
