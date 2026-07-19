#ifndef WINDOW_H
#define WINDOW_H

#include <vector>
#include "DeleteReason.h"
#include "common/tpt-stdint.h"
#include "common/Point.h"
#include "common/SDL_keysym.h"
#include "graphics/Pixel.h"
#include "Component.h"

namespace gfx
{
class VideoBuffer;
}
namespace ui
{
class Window
{
public:
	Window(Point position, Point size);
	virtual ~Window();

	void SetParent(Window *parentWindow) { parent = parentWindow; }
	Window* GetParent() { return parent; }
	void AddSubwindow(Window *other);
	void RemoveSubwindow(Window *other);
	void AddComponent(Component *other);
	void RemoveComponent(Component *other);
	void FocusComponent(Component *toFocus);
	void DefocusComponent(Component *toDefocus);
	void UpdateComponents();
	bool IsFocused(const Component *other) const { return other == focused; }
	bool IsClicked(const Component *other) const { return other == clicked; }

	void DoExit(DeleteReason deleteReason); // calls OnExit, doesn't actually exit though
	void DoFocus();
	void DoDefocus();
	void DoTick(uint32_t ticks, bool isSubWindow);

	virtual void DoDraw(pixel *copyBuf, Point copySize, Point copyPos);
	virtual void DoMouseMove(int x, int y, int dx, int dy);
	virtual void DoMouseDown(int x, int y, unsigned char button);
	virtual void DoMouseUp(int x, int y, unsigned char button);
	virtual void DoMouseWheel(int x, int y, int d);
	virtual void DoKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt);
	virtual void DoKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt);
	virtual void DoTextInput(const char *text);
	virtual void DoJoystickMotion(uint8_t joysticknum, uint8_t axis, int16_t value);
	virtual void DoFileDrop(const char *filename);

	Point GetPosition() { return position; }
	void SetPosition(Point position) { this->position = position; }
	Point GetSize() { return size; }
	void Resize(Point position, Point size);
	gfx::VideoBuffer* GetVid() { return videoBuffer; }
	bool IsMouseDown() { return isMouseDown; }
	bool CanQuit() { return !ignoreQuits; }
	void HasBorder(bool border) { hasBorder = border; }
	int GetTransparency() { return transparency; }
	void SetTransparency(int transparency);

	void SetDoesTextInput(bool doesTextInput) { this->doesTextInput = doesTextInput; }
	void SetLegacyCodeDoesTextInput(bool doesTextInput) { this->legacyCodeDoesTextInput = doesTextInput; }
	bool DoesTextInbpt() { return doesTextInput; }
	bool IsSelfManaged() { return selfManaged; }

	void Close(DeleteReason deleteReason);
	bool IsBeingClosed() { return toDelete; }
	DeleteReason GetDeleteReason() { return deleteReason; }

	static const int CENTERED = -1;

protected:
	gfx::VideoBuffer *videoBuffer;
	Point position;
	Point size;
	std::vector<Component*> Components;
	std::vector<Window*> Subwindows;
	bool isMouseDown; // need to keep track of this for some things like buttons
	bool ignoreQuits;
	bool hasBorder;
	int transparency = 0;
	DeleteReason deleteReason = NoDeleteReason;
	bool toDelete;

	virtual void OnExit(DeleteReason deleteReason) { }
	virtual void OnTick(uint32_t ticks) { }
	virtual void OnDraw(gfx::VideoBuffer *buf) { }
	virtual void OnDrawBeforeComponents(gfx::VideoBuffer *buf) { }
	virtual void OnDrawAfterSubwindows(gfx::VideoBuffer *buf) { }
	virtual void OnMouseMove(int x, int y, Point difference) { }
	virtual void OnMouseDown(int x, int y, unsigned char button) { }
	virtual void OnMouseUp(int x, int y, unsigned char button) { }
	virtual void OnMouseWheel(int x, int y, int d) { }
	virtual void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) { }
	virtual void OnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) { }
	virtual void OnTextInput(const char *text) { }
	virtual void OnJoystickMotion(uint8_t joysticknum, uint8_t axis, int16_t value) { }
	virtual void OnFileDrop(const char *filename) { }

	virtual void OnFocus() { }
	virtual void OnDefocus() { }

	// these functions are called before components are updated, and can be used to cancel events before sending them to the components
	virtual bool BeforeMouseMove(int x, int y, Point difference) { return true; }
	virtual bool BeforeMouseDown(int x, int y, unsigned char button) { return true; }
	virtual bool BeforeMouseUp(int x, int y, unsigned char button) { return true; }
	virtual bool BeforeMouseWheel(int x, int y, int d) { return true; }
	virtual bool BeforeKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) { return true; }
	virtual bool BeforeKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) { return true; }
	virtual bool BeforeTextInput(const char *text) { return true; }

	void VideoBufferHack();
	bool InsideSubwindow(int x, int y);

	Component *clicked = nullptr;
	bool doesTextInput = false;
	bool legacyCodeDoesTextInput = false; // for deco editor
	bool selfManaged = false; // don't automatically delete this window .. major hack for interface api

private:
	Window *parent;
	bool mouseDownOutside;
	Component *focused;
};
}

#endif
