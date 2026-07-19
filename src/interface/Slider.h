#ifndef SLIDER_H
#define SLIDER_H

#include <functional>
#include "common/Point.h"
#include "graphics/ARGBColour.h"
#include "Component.h"

namespace gfx
{
class VideoBuffer;
}

class Slider : public Component
{
	ARGBColour bgColor1 = 0, bgColor2 = 0;
	int steps;
	int sliderPos;
	std::function<void(int)> callback = nullptr;
	bool wasClicked = false;

public:
	Slider(Point position, Point size, int steps, int startPosition = 0);

	void UpdateSliderPos(int position);

	void SetCallback(std::function<void(int)> callback) { this->callback = callback; }

	void SetBackgroundColor(ARGBColour color1, ARGBColour color2) { this->bgColor1 = color1; this->bgColor2 = color2; }
	ARGBColour GetBackgroundColor1() { return bgColor1; }
	ARGBColour GetBackgroundColor2() { return bgColor2; }

	int GetSteps() { return steps; }
	void SetSteps(int steps);
	int GetValue() { return sliderPos; }
	void SetValue(int position);

	void OnDraw(gfx::VideoBuffer* vid) override;
	void OnMouseMoved(int x, int y, Point difference) override;
	void OnMouseDown(int x, int y, unsigned char button) override;
	void OnMouseUp(int x, int y, unsigned char button) override;
};

#endif
