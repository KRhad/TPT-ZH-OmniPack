#include "GravityWindow.h"

#include <sstream>

#include "common/Point.h"
#include "graphics/VideoBuffer.h"
#include "interface/Button.h"
#include "interface/Label.h"
#include "interface/DirectionSelector.h"
#include "interface/Style.h"
#include "simulation/Simulation.h"

GravityWindow::GravityWindow(Simulation *sim, float scale, int radius):
	ui::Window(Point(CENTERED, CENTERED), Point((radius * 5 / 2) + 20, (radius * 5 / 2) + 63)),
	gravityDirection(new DirectionSelector(Point(10, 25), scale, radius, radius / 4, 2, 5)),
	sim(sim)
{
	int x = sim->customGravityX;
	int y = sim->customGravityY;
#ifndef TOUCHUI
	int buttonHeight = 15;
#else
	int buttonHeight = 25;
	Resize(position, size + Point(0, 10));
#endif

	Label * tempLabel = new Label(Point(4, 1), Point(size.X - 8, 22), "Custom Gravity");
	tempLabel->SetColor(COLRGB(140, 140, 255));
	AddComponent(tempLabel);

	std::stringstream gravityText;
	gravityText.precision(1);
	gravityText << std::fixed << "X:" << x << " Y:" << y << " Total:" << std::hypot(x, y);
	Point labelPos = Point((size.X - gfx::VideoBuffer::TextSize(gravityText.str()).X) / 2, (radius * 5 / 2) + 29);
	labelValues = new Label(labelPos, Point(size.X, 16), gravityText.str());
	AddComponent(labelValues);

	gravityDirection->SetValues(x, y);
	gravityDirection->SetUpdateCallback([this, radius](float x, float y) {
		std::stringstream gravityText;
		gravityText.precision(1);
		gravityText << std::fixed << "X:" << x << " Y:" << y << " Total:" << std::hypot(x, y);
		Point labelPos = Point((size.X - gfx::VideoBuffer::TextSize(gravityText.str()).X) / 2, (radius * 5 / 2) + 29);
		labelValues->SetPosition(labelPos);
		labelValues->SetText(gravityText.str());
	});
	gravityDirection->SetSnapPoints(5, 5, 2);
	AddComponent(gravityDirection);

	Button *okButton = new Button(Point(0, size.Y - buttonHeight), Point(size.X, buttonHeight), "OK");
	okButton->SetCallback([&](int mb) {
		this->sim->customGravityX = gravityDirection->GetXValue();
		this->sim->customGravityY = gravityDirection->GetYValue();
	});
	okButton->SetTextColor(COLRGB(140, 140, 255));
	okButton->SetCloseButton(true);
	this->AddComponent(okButton);
}

void GravityWindow::OnDraw(gfx::VideoBuffer* vid)
{
	vid->DrawLine(0, 17, size.X, 17, ui::Style::Border);
}
