#include "ColorPicker.h"

#include <iomanip>

#include "misc.h" // for RGB_to_HSV
#include "common/Format.h"
#include "graphics/VideoBuffer.h"
#include "interface/Button.h"
#include "interface/Label.h"
#include "interface/Slider.h"
#include "interface/Textbox.h"

ColorPicker::ColorPicker(ARGBColour initialColor, std::function<void (int)> callback):
	ui::Window(Point(CENTERED, CENTERED), Point(266, 215)),
	callback(callback)
{
	auto colorChange = [this] {
		int r, g, b, alpha;
		r = Format::StringToNumber<int>(rValue->GetText());
		g = Format::StringToNumber<int>(gValue->GetText());
		b = Format::StringToNumber<int>(bValue->GetText());
		alpha = Format::StringToNumber<int>(aValue->GetText());
		if (r > 255)
			r = 255;
		if (g > 255)
			g = 255;
		if (b > 255)
			b = 255;
		if (alpha > 255)
			alpha = 255;

		RGB_to_HSV(r, g, b, &currentHue, &currentSaturation, &currentValue);
		currentAlpha = alpha;
		UpdateTextboxes(r, g, b, alpha);
		UpdateSliders();
	};

	rValue = new Textbox(Point(5, size.Y-23), Point(30, 17), "255");
	rValue->SetCallback(colorChange);
	rValue->SetCharacterLimit(3);
	rValue->SetType(Textbox::NUMBER);
	this->AddComponent(rValue);

	gValue = new Textbox(Point(40, size.Y-23), Point(30, 17), "255");
	gValue->SetCallback(colorChange);
	gValue->SetCharacterLimit(3);
	gValue->SetType(Textbox::NUMBER);
	this->AddComponent(gValue);

	bValue = new Textbox(Point(75, size.Y-23), Point(30, 17), "255");
	bValue->SetCallback(colorChange);
	bValue->SetCharacterLimit(3);
	bValue->SetType(Textbox::NUMBER);
	this->AddComponent(bValue);

	aValue = new Textbox(Point(110, size.Y-23), Point(30, 17), "255");
	aValue->SetCallback(colorChange);
	aValue->SetCharacterLimit(3);
	aValue->SetType(Textbox::NUMBER);
	this->AddComponent(aValue);

	hexValue = new Label(Point(150, size.Y-23), Point(53, 17), "0xFFFFFFFF");
	this->AddComponent(hexValue);


	auto colourChangeSlider = [this] (int position) {
		int r, g, b;
		currentHue = hSlider->GetValue();
		currentSaturation = sSlider->GetValue();
		currentValue = vSlider->GetValue();

		HSV_to_RGB(currentHue, currentSaturation, currentValue, &r, &g, &b);
		UpdateTextboxes(r, g, b, currentAlpha);
		UpdateSliders();
	};

	hSlider = new Slider(Point(0, 134), Point(size.X, 17), 359);
	hSlider->SetCallback(colourChangeSlider);
	this->AddComponent(hSlider);

	sSlider = new Slider(Point(0, 134 + 17), Point(size.X, 17), 255);
	sSlider->SetCallback(colourChangeSlider);
	this->AddComponent(sSlider);

	vSlider = new Slider(Point(0, 134 + 34), Point(size.X, 17), 255);
	vSlider->SetCallback(colourChangeSlider);
	this->AddComponent(vSlider);

	Button * doneButton = new Button(Point(size.X - 45, size.Y - 23), Point(40, 17), "完成");
	doneButton->SetCallback({ [this] (int mb) {
		if (this->callback)
		{
			int Red = Format::StringToNumber<int>(rValue->GetText());
			int Green = Format::StringToNumber<int>(gValue->GetText());
			int Blue = Format::StringToNumber<int>(bValue->GetText());
			ARGBColour col = COLARGB(currentAlpha, Red, Green, Blue);
			this->callback(col);
		}
	} });
	doneButton->SetCloseButton(true);
	this->AddComponent(doneButton);

	RGB_to_HSV(COLR(initialColor), COLG(initialColor), COLB(initialColor), &currentHue, &currentSaturation, &currentValue);
	currentAlpha = COLA(initialColor);
	UpdateTextboxes(COLR(initialColor), COLG(initialColor), COLB(initialColor), COLA(initialColor));
	UpdateSliders();
}

void ColorPicker::UpdateTextboxes(int r, int g, int b, int a)
{
	rValue->SetText(Format::NumberToString<int>(r));
	gValue->SetText(Format::NumberToString<int>(g));
	bValue->SetText(Format::NumberToString<int>(b));
	aValue->SetText(Format::NumberToString<int>(a));
	std::stringstream hex;
	hex << std::hex << std::uppercase << std::setfill('0');
	hex << std::setw(2) << a;
	hex << std::setw(2) << r;
	hex << std::setw(2) << g;
	hex << std::setw(2) << b;
	hexValue->SetText(hex.str());
}

void ColorPicker::UpdateSliders()
{
	hSlider->SetValue(currentHue);
	sSlider->SetValue(currentSaturation);
	vSlider->SetValue(currentValue);

	int r, g, b;

	//Value gradient
	HSV_to_RGB(currentHue, currentSaturation, 255, &r, &g, &b);
	vSlider->SetBackgroundColor(COLRGB(0, 0, 0), COLRGB(r, g, b));

	//Saturation gradient
	if (currentValue != 0)
	{
		HSV_to_RGB(currentHue, 255, currentValue, &r, &g, &b);
		sSlider->SetBackgroundColor(COLRGB(currentValue, currentValue, currentValue), COLRGB(r, g, b));
	}
}

void ColorPicker::OnDrawBeforeComponents(gfx::VideoBuffer *buf)
{
	buf->DrawRect(4, 4, 258, 130, 180, 180, 180, 255);

	int offsetX = 5;
	int offsetY = 5;

	//draw color square
	int lastx = -1, currx = 0;
	for(int saturation = 0; saturation <= 255; saturation+=2)
	{
		for(int hue = 0; hue <= 359; hue++)
		{
			currx = clamp_flt(float(hue), 0, 359)+offsetX;
			if (currx == lastx)
				continue;
			lastx = currx;
			int cr = 0;
			int cg = 0;
			int cb = 0;
			HSV_to_RGB(hue, 255-saturation, currentValue, &cr, &cg, &cb);
			buf->DrawPixel(currx, (saturation/2)+offsetY, cr, cg, cb, currentAlpha);
		}
	}

	//Draw hue bar gradient
	auto gradientWidth = hSlider->GetSize().X - 10;
	for (int rx = 0; rx < gradientWidth; rx++)
	{
		int red, green, blue;
		int hue = rx * 360 / gradientWidth;
		HSV_to_RGB(hue, currentSaturation, currentValue, &red, &green, &blue);
		for (int ry = 0; ry < (hSlider->GetSize().Y / 2) - 1; ry++)
		{
			buf->DrawPixel(
				rx + offsetX + hSlider->GetPosition().X,
				ry + offsetY + hSlider->GetPosition().Y,
				red, green, blue, currentAlpha
			);
		}
	}

	//draw color square pointer
	int currentHueX = clamp_flt(float(currentHue), 0, 359);
	int currentSaturationY = ((255-currentSaturation)/2);
	buf->XorLine(offsetX+currentHueX, offsetY+currentSaturationY-5, offsetX+currentHueX, offsetY+currentSaturationY-1);
	buf->XorLine(offsetX+currentHueX, offsetY+currentSaturationY+1, offsetX+currentHueX, offsetY+currentSaturationY+5);
	buf->XorLine(offsetX+currentHueX-5, offsetY+currentSaturationY, offsetX+currentHueX-1, offsetY+currentSaturationY);
	buf->XorLine(offsetX+currentHueX+1, offsetY+currentSaturationY, offsetX+currentHueX+5, offsetY+currentSaturationY);
}

void ColorPicker::OnMouseMove(int x, int y, Point difference)
{
	if (mouseDown)
	{
		x -= position.X + 5;
		y -= position.Y + 5;

		currentHue = int((float(x) / float(255)) * 359.0f);
		currentSaturation = 255 - (y * 2);

		if (currentSaturation > 255)
			currentSaturation = 255;
		if (currentSaturation < 0)
			currentSaturation = 0;
		if (currentHue > 359)
			currentHue = 359;
		if (currentHue < 0)
			currentHue = 0;
	}

	if (mouseDown)
	{
		int cr, cg, cb;
		HSV_to_RGB(currentHue, currentSaturation, currentValue, &cr, &cg, &cb);
		UpdateTextboxes(cr, cg, cb, currentAlpha);
		UpdateSliders();
	}
}

void ColorPicker::OnMouseDown(int x, int y, unsigned char button)
{
	x -= position.X + 5;
	y -= position.Y + 5;
	if (x >= 0 && x < 256 && y >= 0 && y <= 128)
	{
		mouseDown = true;
		currentHue = int((float(x) / float(255)) * 359.0f);
		currentSaturation = 255 - (y * 2);

		if (currentSaturation > 255)
			currentSaturation = 255;
		if (currentSaturation < 0)
			currentSaturation = 0;
		if (currentHue > 359)
			currentHue = 359;
		if (currentHue < 0)
			currentHue = 0;
	}

	if (mouseDown)
	{
		int cr, cg, cb;
		HSV_to_RGB(currentHue, currentSaturation, currentValue, &cr, &cg, &cb);
		UpdateTextboxes(cr, cg, cb, currentAlpha);
		UpdateSliders();
	}
}

void ColorPicker::OnMouseUp(int x, int y, unsigned char button)
{
	if (mouseDown)
	{
		int cr, cg, cb;
		HSV_to_RGB(currentHue, currentSaturation, currentValue, &cr, &cg, &cb);
		UpdateTextboxes(cr, cg, cb, currentAlpha);
		UpdateSliders();
	}

	if (mouseDown)
	{
		mouseDown = false;
		x -= position.X + 5;
		y -= position.Y + 5;

		currentHue = int((float(x) / float(255)) * 359.0f);
		currentSaturation = 255 - (y * 2);

		if(currentSaturation > 255)
			currentSaturation = 255;
		if(currentSaturation < 0)
			currentSaturation = 0;
		if(currentHue > 359)
			currentHue = 359;
		if(currentHue < 0)
			currentHue = 0;
	}
}

void ColorPicker::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (repeat)
		return;
	if (key == SDLK_RETURN)
	{
		this->Close(ui::Confirmed);
	}
	else if (key == SDLK_TAB)
	{
		if (rValue->IsFocused())
		{
			FocusComponent(gValue);
			gValue->SelectAll();
		}
		else if (gValue->IsFocused())
		{
			FocusComponent(bValue);
			bValue->SelectAll();
		}
		else if (bValue->IsFocused())
		{
			FocusComponent(aValue);
			aValue->SelectAll();
		}
		else if (aValue->IsFocused())
		{
			FocusComponent(rValue);
			rValue->SelectAll();
		}
	}
}
