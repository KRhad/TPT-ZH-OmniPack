#include "Window.h"
#include "Engine.h"
#include "Label.h"
#include "Style.h"
#include "Textbox.h"
#include "graphics.h"
#include "graphics/VideoBuffer.h"

namespace ui
{

Window::Window(Point position_, Point size_):
	toDelete(false),
	position(position_),
	size(size_),
	Components(std::vector<Component*>()),
	Subwindows(std::vector<Window*>()),
	isMouseDown(false),
	ignoreQuits(false),
	hasBorder(true),
	parent(nullptr),
	mouseDownOutside(false),
	focused(nullptr)
{
	if (position.X == CENTERED)
		position.X = (XRES+BARSIZE-size.X)/2;
	if (position.Y == CENTERED)
		position.Y = (YRES+MENUSIZE-size.Y)/2;
	videoBuffer = new gfx::VideoBuffer(size.X, size.Y);
}

Window::~Window()
{
	delete videoBuffer;
	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
		delete *iter;
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
		delete *iter;
	Components.clear();
	Subwindows.clear();
}

void Window::Resize(Point position, Point size)
{
	delete videoBuffer;
	this->position = position;
	this->size = size;
	if (this->position.X == CENTERED)
		this->position.X = (XRES+BARSIZE-size.X)/2;
	if (this->position.Y == CENTERED)
		this->position.Y = (YRES+MENUSIZE-size.Y)/2;
	// If we are moving or shrinking this window, we need to restore the old video buffer from before showing ourselves
	if (Engine::Ref().GetTop() == this)
		Engine::Ref().RestorePreviousBuffer();
	videoBuffer = new gfx::VideoBuffer(size.X, size.Y);
}

void Window::AddComponent(Component *other)
{
	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		// this component is already part of the window
		if ((*iter) == other)
		{
			return;
		}
	}
	Components.push_back(other);
	other->SetParent(this);
	other->SetVisible(false);
	other->toAdd = true;
}

void Window::AddSubwindow(Window *other)
{
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		// this component is already part of the window
		if ((*iter) == other)
		{
			return;
		}
	}
	Subwindows.push_back(other);
	other->HasBorder(false);
	other->SetParent(this);
}

void Window::RemoveComponent(Component *other)
{
	bool selfManaged = other->IsSelfManaged() || this->selfManaged;
	for (int i = Components.size()-1; i >= 0; i--)
	{
		if (Components[i] == other)
		{
			if (!selfManaged)
				Components[i]->toDelete = true;
			else
			{
				// Hack: Lua will delete this component while garbage collecting
				Components[i]->SetParent(nullptr);
				Components.erase(Components.begin()+i);
			}
		}
	}
	if (other == focused)
		FocusComponent(NULL);
	if (other == clicked)
		clicked = nullptr;
}

void Window::RemoveSubwindow(Window *other)
{
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		if ((*iter) == other)
		{
			(*iter)->toDelete = true;
		}
	}
}

void Window::FocusComponent(Component *toFocus)
{
	if (focused != toFocus)
	{
		if (focused)
			focused->OnDefocus();

		// In unusual situations with subwindows or this window being a subwindow, make sure two components aren't focused at once
		// Minor shortfall in cases of subwindows nested more than 2 layers ... but why would you do that .-.
		for (auto &subwindow : Subwindows)
			if (subwindow->focused != nullptr)
			{
				subwindow->focused->OnDefocus();
				subwindow->focused = nullptr;
			}
		if (this->parent)
			if (this->parent->focused)
			{
				this->parent->focused->OnDefocus();
				this->parent->focused = nullptr;
			}
		focused = toFocus;
		if (focused)
			focused->OnFocus();
	}
}

void Window::DefocusComponent(Component *toDefocus)
{
	if (focused && focused == toDefocus)
	{
		focused->OnDefocus();
		focused = nullptr;
	}
}

void Window::UpdateComponents()
{
	for (int i = Components.size()-1; i >= 0; i--)
	{
		if (Components[i]->toDelete)
		{
			Components[i]->SetParent(NULL);
			delete Components[i];
			Components.erase(Components.begin()+i);
		}
		else if (Components[i]->toAdd)
		{
			Components[i]->SetVisible(true);
		}
	}
	for (int i = Subwindows.size()-1; i >= 0; i--)
	{
		Subwindows[i]->UpdateComponents();
		if (Subwindows[i]->toDelete)
		{
			delete Subwindows[i];
			Subwindows.erase(Subwindows.begin()+i);
		}
	}
}

void Window::SetTransparency(int transparency)
{
	if (transparency < 0)
		transparency = 0;
	else if (transparency > 255)
		transparency = 255;
	this->transparency = transparency;
}

void Window::DoExit(DeleteReason deleteReason)
{
	OnExit(deleteReason);
}

void Window::DoFocus()
{
	OnFocus();
}

void Window::DoDefocus()
{
	OnDefocus();
}

void Window::DoTick(uint32_t ticks, bool isSubWindow)
{
	if (!isSubWindow)
	{
		bool needsTextInput = doesTextInput || legacyCodeDoesTextInput || (focused && focused->IsVisible() && focused->IsEnabled() && focused->DoesTextInput());
		for (auto &subWindow : Subwindows)
		{
			Component *f = subWindow->focused;
			needsTextInput = needsTextInput || (f && f->IsVisible() && f->IsEnabled() && f->DoesTextInput());
		}

		if (needsTextInput)
			Engine::Ref().StartTextInput();
		else
			Engine::Ref().StopTextInput();
	}

	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoTick(ticks, true);
	}
	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		if ((*iter)->IsVisible())
			(*iter)->OnTick(ticks);
	}

	OnTick(ticks);
}

void Window::DoDraw(pixel *copyBuf, Point copySize, Point copyPos)
{
	// too lazy to create another variable which is temporary anyway
	if (!ignoreQuits)
		videoBuffer->Clear();

	OnDrawBeforeComponents(videoBuffer);

	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		if ((*iter)->IsVisible() && !IsFocused(*iter))
			(*iter)->OnDraw(videoBuffer);
	}
	// draw the focused component on top
	if (focused)
		focused->OnDraw(videoBuffer);

	OnDraw(videoBuffer);

	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoDraw(videoBuffer->GetVid(), size, (*iter)->GetPosition());
	}

	OnDrawAfterSubwindows(videoBuffer);

	if (hasBorder)
		videoBuffer->DrawRect(0, 0, size.X, size.Y, Style::Border);
	if (copyBuf)
	{
		if (!transparency)
			videoBuffer->CopyBufferInto(copyBuf, copySize.X, copySize.Y, copyPos.X, copyPos.Y);
		else
		{
			Engine::Ref().RestorePreviousBuffer();
			videoBuffer->CopyBufferIntoWithPartialAlpha(copyBuf, copySize.X, copySize.Y, copyPos.X, copyPos.Y, 255 - transparency);
		}
	}
}

void Window::DoMouseMove(int x, int y, int dx, int dy)
{
	if (!BeforeMouseMove(x, y, Point(dx, dy)))
		return;
	
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoMouseMove(x-position.X, y-position.Y, dx, dy);
	}

	bool alreadyInside = false;
	if (dx || dy)
	{
		for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
		{
			Component *temp = *iter;
			if (temp->IsVisible() && temp->IsEnabled())
			{
				Component *temp = *iter;
				int posX = x-this->position.X-temp->GetPosition().X, posY = y-this->position.Y-temp->GetPosition().Y;
				// update isMouseInside for this component
				if (!alreadyInside && posX >= 0 && posX < temp->GetSize().X && posY >= 0 && posY < temp->GetSize().Y)
				{
					if (!InsideSubwindow(x, y))
						temp->SetMouseInside(true);
					alreadyInside = true;
				}
				else
				{
					temp->SetMouseInside(false);
					if (temp == clicked)
						clicked = nullptr;
				}
				temp->OnMouseMoved(posX, posY, Point(dx, dy));
			}
		}
	}

	OnMouseMove(x, y, Point(dx, dy));
}

void Window::DoMouseDown(int x, int y, unsigned char button)
{
	if (!BeforeMouseDown(x, y, button))
		return;

	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoMouseDown(x - position.X, y - position.Y, button);
	}

	if (InsideSubwindow(x - position.X, y - position.Y))
		return;
	if (x < position.X || x > position.X+size.X || y < position.Y || y > position.Y+size.Y)
	{
#ifndef TOUCHUI
		mouseDownOutside = true;
#endif
		return;
	}
	isMouseDown = true;

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
				temp->OnMouseDown(posX, posY, button);
				break;
			}
		}
	}
	if (!focusedSomething)
		FocusComponent(NULL);

	OnMouseDown(x, y, button);
}

void Window::DoMouseUp(int x, int y, unsigned char button)
{
	if (!BeforeMouseUp(x, y, button))
		return;

	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoMouseUp(x-position.X, y-position.Y, button);
	}

#ifndef TOUCHUI
	if (mouseDownOutside)
	{
		mouseDownOutside = false;
		if (hasBorder && (x < position.X || x > position.X+size.X || y < position.Y || y > position.Y+size.Y))
		{
			toDelete = true;
			deleteReason = MouseOutside;
		}
	}
#endif
	if (InsideSubwindow(x - position.X, y - position.Y))
		return;

	isMouseDown = false;
	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		Component *temp = *iter;
		if (temp->IsVisible() && temp->IsEnabled())
		{
			int posX = x-this->position.X-temp->GetPosition().X, posY = y-this->position.Y-temp->GetPosition().Y;
			bool inside = posX >= 0 && posX < temp->GetSize().X && posY >= 0 && posY < temp->GetSize().Y;

			if (inside || IsClicked(temp) || IsFocused(temp))
				temp->OnMouseUp(posX, posY, button);
		}
	}
	clicked = nullptr;

	OnMouseUp(x, y, button);
}

void Window::DoMouseWheel(int x, int y, int d)
{
	if (!BeforeMouseWheel(x, y, d))
		return;
	
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoMouseWheel(x-position.X, y-position.Y, d);
	}

	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		if ((*iter)->IsVisible() && (*iter)->IsEnabled())
			(*iter)->OnMouseWheel(x, y, d);
	}

	OnMouseWheel(x, y, d);
}

void Window::DoKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (!BeforeKeyPress(key, scan, repeat, shift, ctrl, alt))
		return;

	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoKeyPress(key, scan, repeat, shift, ctrl, alt);
	}
	
	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		if (IsFocused(*iter) && (*iter)->IsVisible() && (*iter)->IsEnabled())
			(*iter)->OnKeyPress(key, scan, repeat, shift, ctrl, alt);
	}

	OnKeyPress(key, scan, repeat, shift, ctrl, alt);
}

void Window::DoKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (!BeforeKeyRelease(key, scan, repeat, shift, ctrl, alt))
		return;
	
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoKeyRelease(key, scan, repeat, shift, ctrl, alt);
	}

	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		if (IsFocused(*iter) && (*iter)->IsVisible() && (*iter)->IsEnabled())
			(*iter)->OnKeyRelease(key, scan, repeat, shift, ctrl, alt);
	}

	OnKeyRelease(key, scan, repeat, shift, ctrl, alt);
}

void Window::DoTextInput(const char *text)
{
	if (!BeforeTextInput(text))
		return;

	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoTextInput(text);
	}

	for (std::vector<Component*>::iterator iter = Components.begin(), end = Components.end(); iter != end; iter++)
	{
		if (IsFocused(*iter) && (*iter)->IsVisible() && (*iter)->IsEnabled())
			(*iter)->OnTextInput(text);
	}

	OnTextInput(text);
}

void Window::DoJoystickMotion(uint8_t joysticknum, uint8_t axis, int16_t value)
{
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		(*iter)->DoJoystickMotion(joysticknum, axis, value);
	}

	OnJoystickMotion(joysticknum, axis, value);
}

void Window::DoFileDrop(const char *filename)
{
	OnFileDrop(filename);
}

void Window::VideoBufferHack()
{
	std::copy(&vid_buf[0], &vid_buf[(XRES+BARSIZE)*(YRES+MENUSIZE)], &videoBuffer->GetVid()[0]);
}

bool Window::InsideSubwindow(int x, int y)
{
	for (std::vector<Window*>::iterator iter = Subwindows.begin(), end = Subwindows.end(); iter != end; iter++)
	{
		Window* wind = (*iter);
		if (Point(x, y).IsInside(wind->GetPosition(), wind->GetPosition()+wind->GetSize()))
		{
			return true;
		}
	}
	return false;
}

}
