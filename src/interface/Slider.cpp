#include "Slider.h"
#include "graphics/VideoBuffer.h"

Slider::Slider(Point position, Point size, int steps, int startPosition):
	Component(position, size),
	steps(steps),
	sliderPos(startPosition)
{

}

void Slider::UpdateSliderPos(int position)
{
	if (position < 3)
		position = 3;
	if (position > size.X - 3)
		position = size.X - 3;

	auto fPosition = float(position - 3);
	auto fSize = float(size.X - 6);

	float fSliderPosition = (fPosition/fSize)*steps;

	auto newSliderPosition = int(fSliderPosition);

	if (newSliderPosition == position)
		return;

	this->sliderPos = newSliderPosition;

	if (callback)
		callback(position);
}

void Slider::SetSteps(int steps)
{
	if (steps < 0)
		steps = 0;
	if (steps < sliderPos)
		sliderPos = steps;
	this->steps = steps;
}

void Slider::SetValue(int value)
{
	if (value < 0)
		value = 0;
	if (value > steps)
		value = steps;
	sliderPos = value;
}

void Slider::OnDraw(gfx::VideoBuffer* vid)
{
	if (bgColor1)
	{
		int width = size.X - 8;
		for (int xx = 2; xx < width; ++xx)
		{
			auto f = xx / (float)width;
			for (int yy = 2; yy < size.Y - 8; ++yy)
			{
				auto rr = int(COLR(bgColor1) * (1.f - f) + COLR(bgColor2) * f);
				auto gg = int(COLG(bgColor1) * (1.f - f) + COLG(bgColor2) * f);
				auto bb = int(COLB(bgColor1) * (1.f - f) + COLB(bgColor2) * f);
				vid->DrawPixel(position.X + xx + 3, position.Y + yy + 3, rr, gg, bb, 255);
			}
		}
	}

	vid->DrawRect(position.X + 3, position.Y + 3, size.X - 6, size.Y - 6, 255, 255, 255, 255);

	auto fPosition = float(sliderPos);
	auto fSize = float(size.X - 6);
	auto fSteps = float(steps);

	auto fSliderX = (fSize / fSteps) * fPosition;
	auto sliderX = int(fSliderX);
	sliderX += 3;

	vid->FillRect(position.X + sliderX - 2, position.Y + 1, 4, size.Y - 2, 20, 20, 20, 255);
	vid->DrawRect(position.X + sliderX - 2, position.Y + 1, 4, size.Y - 2, 200, 200, 200, 255);
}

void Slider::OnMouseDown(int x, int y, unsigned char button)
{
	UpdateSliderPos(x);
	wasClicked = true;
}

void Slider::OnMouseUp(int x, int y, unsigned char button)
{
	wasClicked = false;
}

void Slider::OnMouseMoved(int x, int y, Point difference)
{
	if (wasClicked)
		UpdateSliderPos(x);
}
