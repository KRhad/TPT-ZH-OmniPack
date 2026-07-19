#ifndef GOLWINDOW_H
#define GOLWINDOW_H
#include <string>
#include "interface/Window.h"

class Label;
class Button;
class Textbox;
class GolWindow : public ui::Window
{
	Label *golLabel;
	Button *okButton, *highColorButton, *lowColorButton;
	Textbox *nameTextbox, *ruleTextbox;

	ARGBColour highColor, lowColor;

	bool leftTool;

	void UpdateGradient();
	bool AddGol();
public:
	GolWindow(bool leftTool);
	GolWindow(std::string ruleStr, int color1, int color2, bool leftTool = true);
	~GolWindow();

	void OnDraw(gfx::VideoBuffer *buf) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
