#ifndef SCROLLWINDOW_H
#define SCROLLWINDOW_H

#include "Window.h"
#include <functional>

namespace ui
{
class ScrollWindow : public Window
{
	bool scrollable;
	int scrollSize, scrolled;
	float scrollVelocity = 0.0f, scrolledFloat;
	int lastMouseX, lastMouseY;

	bool isMouseInsideScrollbar = false, scrollbarHeld = false;
	int initialClickX = 0, initialClickY = 0, initialOffset = 0;

	std::function<void(gfx::VideoBuffer*)> onDraw = nullptr;

#ifdef TOUCHUI
	bool hasScrolled = false;
	const int scrollbarWidth = 0;
#else
	const int scrollbarWidth = 8;
#endif

public:
	ScrollWindow(Point position, Point size);

	bool IsScrollable() { return scrollable; }
	int GetUsableWidth() { return size.X - scrollbarWidth; }
	void SetScrollSize(int maxScroll, bool canScrollFollow = false);
	int GetScrollSize() { return scrollSize; }
	int GetScrollPosition() { return scrolled; }
	void SetScrollPosition(int pos);
	void SetOnDraw(std::function<void(gfx::VideoBuffer*)> onDraw) { this->onDraw = onDraw; }

protected:
	void OnDrawBeforeComponents(gfx::VideoBuffer *buf) override;
	void DoMouseDown(int x, int y, unsigned char button) override;
#ifdef TOUCHUI
	void CheckFocus(int x, int y);
	void DoMouseUp(int x, int y, unsigned char button) override;
#endif
	void OnMouseUp(int x, int y, unsigned char button) override;
	void DoMouseWheel(int x, int y, int d) override;
	void OnTick(uint32_t ticks) override;
	void OnDraw(gfx::VideoBuffer *buf) override;
	void OnMouseMove(int x, int y, Point difference) override;
};
}
#endif
