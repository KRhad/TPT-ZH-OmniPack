#include "ScrollWindow.h"
#include "defines.h"
#include "common/tpt-minmax.h"
#include "graphics/VideoBuffer.h"
#include "interface/Engine.h"

namespace ui
{
ScrollWindow::ScrollWindow(Point position, Point size):
	Window(position, size),
	scrollable(false),
	scrollSize(0),
	scrolled(0),
	scrolledFloat(0.0f),
	lastMouseX(0),
	lastMouseY(0)
{
}

void ScrollWindow::DoMouseWheel(int x, int y, int d)
{
	if (scrollable)
	{
		lastMouseX = x;
		lastMouseY = y;
		if (Engine::Ref().IsMomentumScroll())
			scrollVelocity -= d * scrollSpeedMomentum;
		else
			scrollVelocity -= d * scrollSpeed;
	}

	/*for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		(*iter)->OnMouseWheel(x, y, d);
	}*/

	OnMouseWheel(x, y, d);
}

void ScrollWindow::OnDrawBeforeComponents(gfx::VideoBuffer *buf)
{
	if (onDraw != nullptr)
		onDraw(buf);
}

void ScrollWindow::OnTick(uint32_t ticks)
{
	int oldScrolled = scrolledFloat;
	scrolledFloat += scrollVelocity;

	if (Engine::Ref().IsMomentumScroll())
	{
		if (scrollVelocity > -0.5f && scrollVelocity < 0.5f)
			scrollVelocity = 0;
		scrollVelocity *= scrollDeceleration;
	}
	else
		scrollVelocity = 0.0f;

	if (int(scrolledFloat) != oldScrolled)
	{
		if (int(scrolledFloat) > scrollSize - size.Y)
		{
			scrolledFloat = scrollSize - size.Y;
			scrollVelocity = 0.0f;
		}
		else if (int(scrolledFloat) < 0)
		{
			scrolledFloat = 0.0f;
			scrollVelocity = 0.0f;
		}
		SetScrollPosition(int(scrolledFloat));
	}
	
}

void ScrollWindow::OnDraw(gfx::VideoBuffer *buf)
{
	if (scrollable)
	{
		float scrollHeight = static_cast<float>(size.Y) * (static_cast<float>(size.Y) / static_cast<float>(scrollSize));
		float scrollPos = 0;
		if (scrolled > 0)
			scrollPos = static_cast<float>(size.Y - scrollHeight) * (static_cast<float>(scrolled) / static_cast<float>(scrollSize - size.Y));

		buf->FillRect(size.X - scrollbarWidth, 0, scrollbarWidth, size.Y, 125, 125, 125, 100);
		buf->FillRect(size.X - scrollbarWidth, static_cast<int>(scrollPos), scrollbarWidth, static_cast<int>(scrollHeight) + 1, 255, 255, 255, 255);
	}
}

void ScrollWindow::DoMouseDown(int x, int y, unsigned char button)
{
	if (isMouseInsideScrollbar)
	{
		// This if statement fixes a bug on the Android port when using the dropdowns in the Options menu
		// Usually you can't be inside the scrollbar and a subwindow at once, but in the touch interface, the entire scrollwindow acts as
		// the scrollbar, so we need to prevent this
		if (!InsideSubwindow(x - position.X, y - position.Y))
			scrollbarHeld = true;
		initialClickX = x;
		initialClickY = y;
		initialOffset = scrolled;
	}

#ifndef TOUCHUI
	Window::DoMouseDown(x, y, button);
#else
	if (!isMouseInsideScrollbar)
		Window::DoMouseDown(x, y, button);
	else
		CheckFocus(x, y);
#endif
}

#ifdef TOUCHUI
// In touch interface, the mousedown event doesn't happen until mouse up
// We still want components to be focused / clicked before then, so set that manually here
void ScrollWindow::CheckFocus(int x, int y)
{
	if (InsideSubwindow(x - position.X, y - position.Y))
		return;
	if (x < position.X || x > position.X+size.X || y < position.Y || y > position.Y+size.Y)
		return;

	bool focusedSomething = false;
	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		Component *temp = *iter;
		if (temp->IsVisible() && temp->IsEnabled())
		{
			int posX = x-this->position.X-temp->GetPosition().X, posY = y-this->position.Y-temp->GetPosition().Y;
			bool inside = posX >= 0 && posX < temp->GetSize().X && posY >= 0 && posY < temp->GetSize().Y;
			if (inside)
			{
				focusedSomething = true;
				FocusComponent(temp);
				clicked = temp;
				break;
			}
		}
	}
	if (!focusedSomething)
		FocusComponent(NULL);
}

void ScrollWindow::DoMouseUp(int x, int y, unsigned char button)
{
	if (!hasScrolled)
	{
		Window::DoMouseDown(initialClickX, initialClickY, button);
		Window::DoMouseMove(x, y, x - initialClickX, y - initialClickY);
	}
	hasScrolled = false;
	Window::DoMouseUp(x, y, button);
}
#endif

void ScrollWindow::OnMouseUp(int x, int y, unsigned char button)
{
	scrollbarHeld = false;
}

void ScrollWindow::OnMouseMove(int x, int y, Point difference)
{
	if (scrollable)
	{
#ifndef TOUCHUI
		float scrollHeight = static_cast<float>(size.Y) * (static_cast<float>(size.Y) / static_cast<float>(scrollSize));
		float scrollPos = 0;
		if (scrolled > 0)
			scrollPos = static_cast<float>(size.Y - scrollHeight) * (static_cast<float>(scrolled) / static_cast<float>(scrollSize - size.Y));
		scrollPos += this->position.Y;

		isMouseInsideScrollbar = x >= size.X - scrollbarWidth && x < size.X && y >= scrollPos && y < scrollPos + scrollHeight;
#else
		isMouseInsideScrollbar = true;
#endif

		if (scrollbarHeld && Subwindows.size() == 0)
		{
			int newScrolled;
			if (x < initialClickX - 100)
				newScrolled = initialOffset;
			else
			{
#ifndef TOUCHUI
				newScrolled = static_cast<float>(y - initialClickY) / static_cast<float>(size.Y) * scrollSize + initialOffset;
#else
				// 10 pixels of leeway before it starts scrolling
				int diff = initialClickY - y;
				if (!hasScrolled)
				{
					if (std::abs(diff) < 10)
						diff = 0;
					else if (diff < 0)
						diff += 10;
					else if (diff > 0)
						diff -= 10;
					// Block events from being sent to components once we enter scrolling mode
					if (diff != 0)
					{
						initialClickX = x;
						initialClickY = y;
						hasScrolled = true;
						diff = 0;
					}
				}
				newScrolled = static_cast<float>(diff) / static_cast<float>(size.Y) * scrollSize + initialOffset;
#endif
			}
			lastMouseX = x;
			lastMouseY = y;
			SetScrollPosition(newScrolled);
		}
	}
}

void ScrollWindow::SetScrollSize(int maxScroll, bool canScrollFollow)
{
	// All components in the scroll window fit - disable scrolling
	if (maxScroll - size.Y <= 0)
	{
		scrollable = false;
		scrolled = scrolledFloat = 0;
		scrollSize = maxScroll;
		return;
	}
	// If scroll window is expanding, automatically keep scrollbar attached to bottom
	bool scrollFollow = canScrollFollow && (!scrollable || scrolled + size.Y == scrollSize);

	scrollable = true;
	scrollSize = maxScroll;

	// Keep scrollbar attached to bottom if it was there before
	if (scrollFollow)
		SetScrollPosition(scrollSize - size.Y);

	if (!scrollable)
		SetScrollPosition(0);
	else if (scrolled > scrollSize)
		SetScrollPosition(scrollSize);
}

void ScrollWindow::SetScrollPosition(int pos)
{
	bool alreadyInside = false;
	int oldScrolled = scrolled;

	if (!scrollable)
		return;
	if (pos < 0)
		pos = 0;
	if (pos > scrollSize - size.Y)
		pos = scrollSize - size.Y;
	if (scrolled == pos)
		return;

	scrolled = pos;
	scrolledFloat = pos;
	for (auto &comp : Components)
	{
		comp->SetPosition(Point(comp->GetPosition().X, comp->GetPosition().Y - (scrolled - oldScrolled)));

		int posX = lastMouseX - this->position.X - comp->GetPosition().X, posY = lastMouseY - this->position.Y - comp->GetPosition().Y;
		// update isMouseInside for this component
		if (!alreadyInside && posX >= 0 && posX < comp->GetSize().X && posY >= 0 && posY < comp->GetSize().Y)
		{
			if (!InsideSubwindow(lastMouseX, lastMouseY))
				comp->SetMouseInside(true);
			alreadyInside = true;
		}
		else
			comp->SetMouseInside(false);
	}

	for (auto &sub : Subwindows)
		sub->SetPosition(Point(sub->GetPosition().X, sub->GetPosition().Y - (scrolled - oldScrolled)));
}
}
