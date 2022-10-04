#ifndef GRAVITYWINDOW_H
#define GRAVITYWINDOW_H
#include "interface/Window.h"

class DirectionSelector;
class Label;
class Simulation;
class GravityWindow : public ui::Window
{
	DirectionSelector *gravityDirection;
	Label *labelValues;
	Simulation *sim;
public:
	GravityWindow(Simulation *sim, float scale, int radius);

	void OnDraw(gfx::VideoBuffer* vid) override;
};

#endif
