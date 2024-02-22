#include "GolWindow.h"
#include "common/tpt-rand.h"
#include "game/Menus.h"
#include "graphics/VideoBuffer.h"
#include "interface/Button.h"
#include "interface/Engine.h"
#include "interface/Label.h"
#include "interface/Textbox.h"
#include "gui/colorpicker/ColorPicker.h"
#include "gui/dialogs/ErrorPrompt.h"
#include "simulation/elements/LIFE.h"

GolWindow::GolWindow(bool leftTool):
	GolWindow("", 0, 0, leftTool)
{}

GolWindow::GolWindow(std::string ruleStr, int color1, int color2, bool leftTool):
	ui::Window(Point(CENTERED, CENTERED), Point(200, 98)),
	leftTool(leftTool)
{
	highColor = color1 ? color1 : COLRGB(RNG::Ref().between(0x80, 0xFF), RNG::Ref().between(0x80, 0xFF), RNG::Ref().between(0x80, 0xFF));
	lowColor = color2 ? color2 : COLRGB(RNG::Ref().between(0x00, 0x7F), RNG::Ref().between(0x00, 0x7F), RNG::Ref().between(0x00, 0x7F));

	golLabel = new Label(Point(5, 3), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Edit custom GOL type");
	golLabel->SetColor(COLRGB(140, 140, 255));
	this->AddComponent(golLabel);

	nameTextbox = new Textbox(golLabel->Below(Point(0, 2)), Point(this->size.X-10, Textbox::AUTOSIZE),
							  ruleStr.size() ? "" : static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).GetCachedName());
	nameTextbox->SetCharacterLimit(7);
	this->AddComponent(nameTextbox);

	ruleTextbox = new Textbox(nameTextbox->Below(Point(0, 4)), Point(this->size.X-10, Textbox::AUTOSIZE),
							  ruleStr.size() ? ruleStr : static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).GetCachedRuleString());
	this->AddComponent(ruleTextbox);

	highColorButton = new Button(ruleTextbox->Below(Point(0, 4)), Point(16, 16), "");
	highColorButton->SetCallback([&](int mb) {
		ColorPicker *colorPicker = new ColorPicker(highColor, [this](ARGBColour color) {
			highColor = color;
			UpdateGradient();
		});
		Engine::Ref().ShowWindow(colorPicker);
	});
	this->AddComponent(highColorButton);

	lowColorButton = new Button(ruleTextbox->Below(Point(0, 4)), Point(16, 16), "");
	lowColorButton->SetPosition(Point(this->size.X - lowColorButton->GetSize().X - 5, lowColorButton->GetPosition().Y));
	lowColorButton->SetCallback([&](int mb) {
		ColorPicker *colorPicker = new ColorPicker(lowColor, [this](ARGBColour color) {
			lowColor = color;
			UpdateGradient();
		});
		Engine::Ref().ShowWindow(colorPicker);
	});
	this->AddComponent(lowColorButton);

	okButton = new Button(Point(0, this->size.Y-15), Point(this->size.X+1, 15), "OK");
	okButton->SetCallback([&](int mb) { this->AddGol(); });
	okButton->SetCloseButton(true);
	this->AddComponent(okButton);

	UpdateGradient();
}

GolWindow::~GolWindow()
{
	static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).SetCachedName(nameTextbox->GetText());
	static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).SetCachedRuleString(ruleTextbox->GetText());
}

void GolWindow::UpdateGradient()
{
	highColorButton->SetColor(highColor);
	highColorButton->SetBackgroundColor(highColor);
	lowColorButton->SetColor(lowColor);
	lowColorButton->SetBackgroundColor(lowColor);
}

bool GolWindow::AddGol()
{
	auto name = nameTextbox->GetText();
	auto ruleString = ruleTextbox->GetText();
	if (name.empty() || !ValidateGOLName(name))
	{
		ErrorPrompt *errorPrompt = new ErrorPrompt("Invalid name provided");
		Engine::Ref().ShowWindow(errorPrompt);
		return false;
	}
	int rule = ParseGOLString(ruleString);
	if (rule == -1)
	{
		ErrorPrompt *errorPrompt = new ErrorPrompt("Invalid rule provided");
		Engine::Ref().ShowWindow(errorPrompt);
		return false;
	}
	if (static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).GetCustomGOLByRule(rule))
	{
		ErrorPrompt *errorPrompt = new ErrorPrompt("This Custom GoL rule already exists");
		Engine::Ref().ShowWindow(errorPrompt);
		return false;
	}
	ruleString = SerialiseGOLRule(rule); // * Make it canonical.

	CustomGOLData cgol;
	cgol.color1 = COLMODALPHA(lowColor, 0);
	cgol.color2 = COLMODALPHA(highColor, 0);
	cgol.rule = rule;
	cgol.ruleString = ruleString;
	cgol.nameString = name;
	if (!static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).AddCustomGOL(cgol))
	{
		ErrorPrompt *errorPrompt = new ErrorPrompt("Duplicate name, cannot add");
		Engine::Ref().ShowWindow(errorPrompt);
		return false;
	}
	else
	{
		FillMenus();
		Tool *golTool = GetToolFromIdentifier("DEFAULT_PT_LIFECUST_" + cgol.nameString);
		if (golTool)
			activeTools[leftTool ? 0 : 1] = golTool;
	}

	return true;
}

void GolWindow::OnDraw(gfx::VideoBuffer *buf)
{
	int leftX = highColorButton->Right(Point(6, 0)).X;
	int topX = highColorButton->GetPosition().Y;
	int rightX = lowColorButton->Left(Point(6, 0)).X;

	int width = rightX - leftX;
	for (int xx = 0; xx < width; ++xx)
	{
		auto f = xx / (float)width;
		for (int yy = 0; yy < highColorButton->GetSize().Y; ++yy)
		{
			auto rr = int(COLR(highColor) * (1.f - f) + COLR(lowColor) * f);
			auto gg = int(COLG(highColor) * (1.f - f) + COLG(lowColor) * f);
			auto bb = int(COLB(highColor) * (1.f - f) + COLB(lowColor) * f);
			buf->DrawPixel(xx + leftX, yy + topX, rr, gg, bb, 255);
		}
	}
}

void GolWindow::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (key == SDLK_RETURN)
	{
		if (AddGol())
			Close(ui::Confirmed);
	}
}
