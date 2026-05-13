#ifndef GRAVITYWINDOW_H
#define GRAVITYWINDOW_H

#include <functional>
#include <string>
#include "interface/Window.h"

class DirectionSelector;
class Label;
class Simulation;
class GravityWindow : public ui::Window
{
	DirectionSelector *directionSelector;
	Label *labelValues;
	std::function<void(float, float)> callback = nullptr;

public:
	GravityWindow(float scale, int radius, float x, float y, std::string label, std::function<void(float, float)> callback);

	void OnDraw(gfx::VideoBuffer* vid) override;
};

#endif
