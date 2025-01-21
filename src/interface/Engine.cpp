#include "common/tpt-minmax.h"
#include "graphics.h"
#include "Engine.h"
#include "EventLoopSDL.h"
#include "Window.h"
#include "common/Point.h"

/**
 * Engine class - manages shown windows and communicates with backend i/o libraries (sdl only right now)
 */

Engine::Engine():
	windows(std::stack<ui::Window*>()),
	top(nullptr),
	nextTop(nullptr)
{

}

Engine::~Engine()
{
	while (!windows.empty())
	{
		delete windows.top();
		windows.pop();
	}
	while (!buffers.empty())
	{
		delete[] buffers.top();
		buffers.pop();
	}
}

void Engine::ProcessWindowUpdates()
{
	// close / open any windows that need to be shown
	CloseWindowDelayed();
	if (nextTop)
		ShowWindowDelayed();
}

// can only show one new window per frame
void Engine::ShowWindow(ui::Window *window)
{
	if (!window || nextTop)
		return;
	nextTop = window;
	// show window now if this is the first
	if (!top)
		ShowWindowDelayed();
}

void Engine::ShowWindowDelayed()
{
	if (top)
		top->DoDefocus();

	fillrect(vid_buf, -1, -1, XRES+BARSIZE+1, YRES+MENUSIZE+1, 0, 0, 0, 100);

	// Make a copy of the video buffer, for restoration when this window is closed
	pixel *copy = new pixel[VIDXRES * VIDYRES * PIXELSIZE];
	std::copy(&vid_buf[0], &vid_buf[VIDXRES * VIDYRES], &copy[0]);
	buffers.push(copy);

	windows.push(nextTop);
	top = nextTop;
	nextTop = NULL;
	top->DoFocus();

	// update mouse position on any new windows
	int mx, my;
	CalculateMousePosition(&mx, &my);
	top->DoMouseMove(mx, my, 0, 0);
}

void Engine::CloseTop(ui::DeleteReason deleteReason)
{
	ui::Window *temp = windows.top();
	temp->Close(deleteReason);
}

void Engine::CloseWindowDelayed()
{
	while (top && (top->IsBeingClosed() || isShutdown))
	{
		top->DoExit(top->GetDeleteReason());
		if (!top->IsSelfManaged())
			delete top;
		else
			top->UpdateComponents();
		windows.pop();

		// Restore original copy of the video buffer from before this window was shown
		pixel *copy = buffers.top();
		buffers.pop();
		std::copy(&copy[0], &copy[VIDXRES * VIDYRES], &vid_buf[0]);
		delete[] copy;

		if (windows.size())
		{
			top = windows.top();
			top->FocusComponent(NULL);
			// update mouse position on any new windows
			int mx, my;
			CalculateMousePosition(&mx, &my);
			top->DoMouseMove(mx, my, 0, 0);
		}
		else
			top = NULL;

		// Limit to one window close per frame in secondary loops .. kind of a hack to prevent crashing
		if (secondaryEngineLoop)
			break;
	}
}

// Called when a Window is resizing itself. If it's making itself smaller, needs to restore the old buffer
void Engine::RestorePreviousBuffer()
{
	pixel *copy = buffers.top();
	std::copy(&copy[0], &copy[VIDXRES * VIDYRES], &vid_buf[0]);
}

unsigned int Engine::GetScale()
{
	return scale;
}

void Engine::SetScale(unsigned int scale)
{
	if (this->scale != scale)
	{
		this->scale = scale;
		SDLSetScreen(resizable, pixelFilteringMode, fullscreen, altFullscreen, forceIntegerScaling, true);
	}
}

constexpr int SCALE_MAXIMUM = 10;
constexpr int SCALE_MARGIN = 30;

int Engine::GuessBestScale()
{
	const int widthNoMargin = screenWidth - SCALE_MARGIN;
	const int widthGuess = widthNoMargin / VIDXRES;

	const int heightNoMargin = screenHeight - SCALE_MARGIN;
	const int heightGuess = heightNoMargin / VIDYRES;

	int guess = std::min(widthGuess, heightGuess);
	if(guess < 1 || guess > SCALE_MAXIMUM)
		guess = 1;

	return guess;
}

bool Engine::IsResizable()
{
	return resizable;
}

void Engine::SetResizable(bool resizable, bool recreateWindow)
{
	if (this->resizable != resizable)
	{
		SDLSetScreen(resizable, pixelFilteringMode, fullscreen, altFullscreen, forceIntegerScaling, recreateWindow);
		this->resizable = resizable;
	}
}

int Engine::GetPixelFilteringMode()
{
	return pixelFilteringMode;
}

void Engine::SetPixelFilteringMode(int pixelFilteringMode, bool recreateWindow)
{
	if (this->pixelFilteringMode != pixelFilteringMode)
	{
		this->pixelFilteringMode = pixelFilteringMode;
		SDLSetScreen(resizable, pixelFilteringMode, fullscreen, altFullscreen, forceIntegerScaling, recreateWindow);
	}
}

bool Engine::IsFullscreen()
{
	return fullscreen;
}

void Engine::SetFullscreen(bool fullscreen)
{
	if (this->fullscreen != fullscreen)
	{
		SDLSetScreen(resizable, pixelFilteringMode, fullscreen, altFullscreen, forceIntegerScaling, true);
		this->fullscreen = fullscreen;
	}
}

bool Engine::IsAltFullscreen()
{
	return altFullscreen;
}

void Engine::SetAltFullscreen(bool altFullscreen)
{
	if (this->altFullscreen != altFullscreen)
	{
		SDLSetScreen(resizable, pixelFilteringMode, fullscreen, altFullscreen, forceIntegerScaling, true);
		this->altFullscreen = altFullscreen;
	}
}

bool Engine::IsForceIntegerScaling()
{
	return forceIntegerScaling;
}

void Engine::SetForceIntegerScaling(bool forceIntegerScaling)
{
	if (this->forceIntegerScaling != forceIntegerScaling)
	{
		SDLSetScreen(resizable, pixelFilteringMode, fullscreen, altFullscreen, forceIntegerScaling, true);
		this->forceIntegerScaling = forceIntegerScaling;
	}
}

void Engine::StartTextInput()
{
	if (textInput)
	{
		return;
	}
	textInput = true;
	::StartTextInput();
}

void Engine::StopTextInput()
{
	if (!textInput)
	{
		return;
	}
	::StopTextInput();
	textInput = false;
}

void Engine::ClipboardPush(std::string text)
{
	PushToClipboard(text);
}

std::string Engine::ClipboardPull()
{
	return PullFromClipboard();
}

int Engine::GetModifiers()
{
	return SDLGetModifiers();
}
