#ifndef COMPONENT_H
#define COMPONENT_H

#include "graphics/ARGBColour.h"
#include "common/tpt-stdint.h"
#include "common/Point.h"
#include "common/SDL_keysym.h"

namespace gfx
{
class VideoBuffer;
}
namespace ui
{
	class Window;
}
class Component
{
	ui::Window *parent;

protected:
	Point position;
	Point size;
	bool isMouseInside; // keeps track of if mouse is inside so child classes don't have to
	bool visible; // ignore all events + tick and hide component
	bool enabled; // ignore all events, still call tick and draw function
	bool doesTextInput = false; // Should textinput events be forwarded to this component?
	ARGBColour color;
	bool selfManaged = false;

public:
	Component(Point position, Point size);
	virtual ~Component() { }

	// delay deleting and adding components
	bool toDelete;
	bool toAdd;

	bool IsFocused();
	bool IsClicked();
	bool IsMouseDown();

	void SetParent(ui::Window *parentWindow) { parent = parentWindow; }
	ui::Window* GetParent() { return parent; }
	Point GetPosition() { return position; }
	void SetPosition(Point position) { this->position = position; }
	Point GetSize() { return size; }
	virtual void SetSize(Point size) { this->size = size; }
	void SetMouseInside(bool mouseInside) { isMouseInside = mouseInside; } // used by Window.cpp
	bool IsVisible() { return visible; }
	void SetVisible(bool visible);
	bool IsEnabled() { return enabled; }
	void SetEnabled(bool enabled_) { enabled = enabled_; if (!enabled) isMouseInside = false; }
	bool DoesTextInput() { return doesTextInput; }
	void SetDoesTextInput(bool doesTextInput) { this->doesTextInput = doesTextInput; }
	virtual void SetColor(ARGBColour newColor) { color = newColor; }
	ARGBColour GetColor() { return color; }
	bool IsSelfManaged() { return selfManaged; }
	void SetSelfManaged() { selfManaged = true; }

	// can be used to line up components
	Point Right(Point diff) { return Point(position.X + size.X + diff.X, position.Y+diff.Y); }
	Point Left(Point diff) { return Point(position.X - diff.X, position.Y - diff.Y); }
	Point Above(Point diff) { return Point(position.X - diff.X, position.Y - diff.Y); }
	Point Below(Point diff) { return Point(position.X + diff.X, position.Y + size.Y + diff.Y); }

	virtual void OnTick(uint32_t ticks) { }
	virtual void OnDraw(gfx::VideoBuffer* vid) { }
	virtual void OnMouseMoved(int x, int y, Point difference) { }
	virtual void OnMouseDown(int x, int y, unsigned char button) { }
	virtual void OnMouseUp(int x, int y, unsigned char button) { }
	virtual void OnMouseWheel(int x, int y, int d) { }
	virtual void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) { }
	virtual void OnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) { }
	virtual void OnTextInput(const char *text) { }

	virtual void OnFocus() { }
	virtual void OnDefocus() { }
};

#endif
