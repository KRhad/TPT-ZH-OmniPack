#ifndef SDL1_2
#include <climits>
#include <list>

#include "SDLCompat.h"
#ifdef LIN
#include "images.h"
#endif

#include "EventLoopSDL.h"
#include "defines.h"
#include "graphics.h"
#include "interface.h"

#include "common/Platform.h"
#include "common/tpt-minmax.h"
#include "interface/Engine.h"
#include "gui/dialogs/ConfirmPrompt.h"
#include "gui/game/PowderToy.h" // for the_game->DeFocus(), remove once all interfaces get modernized


SDL_Window * sdl_window = nullptr;
SDL_Renderer * sdl_renderer = nullptr;
SDL_Texture * sdl_texture = nullptr;

bool resizable = false;
int pixelFilteringMode = 0;
bool fullscreen = false;
bool altFullscreen = false;
bool forceIntegerScaling = true;

int savedWindowX = INT_MAX;
int savedWindowY = INT_MAX;

unsigned short sdl_mod;
std::string sdl_textinput = "";
int sdl_key, sdl_wheel;

void StartTextInput()
{
	SDL_StartTextInput();
}

void StopTextInput()
{
	SDL_StopTextInput();
}

void LoadWindowPosition()
{
	int borderTop, borderLeft;
	SDL_GetWindowBordersSize(sdl_window, &borderTop, &borderLeft, nullptr, nullptr);
	// Sometimes (Windows), the border size may not be reported for 200+ frames
	// So just have a default of 5 to ensure the window doesn't get stuck where it can't be moved
	if (borderTop == 0)
		borderTop = 5;

	int numDisplays = SDL_GetNumVideoDisplays();
	SDL_Rect displayBounds;
	bool ok = false;
	for (int i = 0; i < numDisplays; i++)
	{
		SDL_GetDisplayBounds(i, &displayBounds);
		if (savedWindowX + borderTop > displayBounds.x && savedWindowY + borderLeft > displayBounds.y &&
				savedWindowX + borderTop < displayBounds.x + displayBounds.w &&
				savedWindowY + borderLeft < displayBounds.y + displayBounds.h)
		{
			ok = true;
			break;
		}
	}
	if (ok)
		SDL_SetWindowPosition(sdl_window, savedWindowX + borderLeft, savedWindowY + borderTop);
}

void SaveWindowPosition()
{
	int x, y;
	SDL_GetWindowPosition(sdl_window, &x, &y);

	int borderTop, borderLeft;
	SDL_GetWindowBordersSize(sdl_window, &borderTop, &borderLeft, nullptr, nullptr);

	savedWindowX = x - borderLeft;
	savedWindowY = y - borderTop;
}

void SDL_Quit_Wrapper()
{
	if (SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_OPENGL)
	{
		// * nvidia-460 egl registers callbacks with x11 that end up being called
		//   after egl is unloaded unless we grab it here and release it after
		//   sdl closes the display. this is an nvidia driver weirdness but
		//   technically an sdl bug. glfw has this fixed:
		//   https://github.com/glfw/glfw/commit/9e6c0c747be838d1f3dc38c2924a47a42416c081
		SDL_GL_LoadLibrary(NULL);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_GL_UnloadLibrary();
	}

	SDL_Quit();
}

int sdl_opened = 0;
bool RecreateWindow();
void SDLInit()
{
	// https://bugzilla.libsdl.org/show_bug.cgi?id=3796
	if (SDL_Init(0) < 0)
	{
		fprintf(stderr, "Initializing SDL: %s\n", SDL_GetError());
		exit(-1);
	}
	atexit(SDL_Quit_Wrapper);
}

int SDLOpen()
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Initializing SDL (video subsystem): %s\n", SDL_GetError());
		return 0;
	}

	if (!RecreateWindow())
	{
		fprintf(stderr, "Creating SDL window: %s\n", SDL_GetError());
		exit(-1);
	}

	if (!sdl_opened)
	{
		int displayIndex = SDL_GetWindowDisplayIndex(sdl_window);
		if (displayIndex >= 0)
		{
			SDL_Rect rect;
			if (!SDL_GetDisplayUsableBounds(displayIndex, &rect))
			{
				screenWidth = rect.w;
				screenHeight = rect.h;
			}
		}
	}

#ifdef ANDROID
	// hide overlay buttons that just get in the way
	SDL_ANDROID_SetScreenKeyboardShown(0);
	//SDL_JoystickOpen(1);
#endif

#ifdef WIN
	SDL_SysWMinfo SysInfo;
	SDL_VERSION(&SysInfo.version);
	if(SDL_GetWindowWMInfo(sdl_window, &SysInfo) <= 0)
	{
	    printf("%s : %p\n", SDL_GetError(), SysInfo.info.win.window);
	    exit(-1);
	}
	HWND WindowHandle = SysInfo.info.win.window;

	// Use GetModuleHandle to get the Exe HMODULE/HINSTANCE
	HMODULE hModExe = GetModuleHandle(NULL);
	HICON hIconSmall = (HICON)LoadImage(hModExe, MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_SHARED);
	HICON hIconBig = (HICON)LoadImage(hModExe, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_SHARED);
	SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
	SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
#endif
#ifdef LIN
	SDL_Surface *icon = SDL_CreateRGBSurfaceFrom((void*)app_icon, 128, 128, 32, 512, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	SDL_SetWindowIcon(sdl_window, icon);
	SDL_FreeSurface(icon);
#endif

	sdl_opened = 1;

	return 1;
}

void UpdateSettings(bool resizable_, int pixelFilteringMode_, bool fullscreen_, bool altFullscreen_, bool forceIntegerScaling_)
{
	resizable = resizable_;
	pixelFilteringMode = pixelFilteringMode_;
	fullscreen = fullscreen_;
	altFullscreen = altFullscreen_;
	forceIntegerScaling = forceIntegerScaling_;
}

void SDLSetScreen(bool resizable_, int pixelFilteringMode_, bool fullscreen_, bool altFullscreen_,
				  bool forceIntegerScaling_, bool canRecreateWindow)
{
	if (!sdl_opened)
	{
		UpdateSettings(resizable_, pixelFilteringMode_, fullscreen_, altFullscreen_, forceIntegerScaling_);
		return;
	}
	bool changingFullscreen = fullscreen_ != fullscreen || (altFullscreen_ != altFullscreen && fullscreen);
	bool changingResizable = resizable != resizable_ || pixelFilteringMode != pixelFilteringMode_;
	UpdateSettings(resizable_, pixelFilteringMode_, fullscreen_, altFullscreen_, forceIntegerScaling_);
	if ((changingFullscreen || altFullscreen || (changingResizable && resizable && !fullscreen)) && canRecreateWindow)
	{
		RecreateWindow();
		return;
	}
	if (changingResizable && canRecreateWindow)
		SDL_RestoreWindow(sdl_window);

	SDL_SetWindowSize(sdl_window, VIDXRES * Engine::Ref().GetScale(), VIDYRES * Engine::Ref().GetScale());
	SDL_RenderSetIntegerScale(sdl_renderer, forceIntegerScaling && fullscreen ? SDL_TRUE : SDL_FALSE);
	unsigned int flags = 0;
	if (fullscreen)
		flags = altFullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP;
	SDL_SetWindowFullscreen(sdl_window, flags);
	if (fullscreen)
		SDL_RaiseWindow(sdl_window);
	SDL_SetWindowResizable(sdl_window, resizable ? SDL_TRUE : SDL_FALSE);
}

bool RecreateWindow()
{
	unsigned int flags = 0;
	if (fullscreen)
		flags = altFullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (resizable)
		flags |= SDL_WINDOW_RESIZABLE;

	if (sdl_texture)
		SDL_DestroyTexture(sdl_texture);
	if (sdl_renderer)
		SDL_DestroyRenderer(sdl_renderer);
	if (sdl_window)
	{
		SaveWindowPosition();
		SDL_DestroyWindow(sdl_window);
	}

	sdl_window = SDL_CreateWindow("Jacob1'sMod", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	        VIDXRES * Engine::Ref().GetScale(), VIDYRES * Engine::Ref().GetScale(), flags);
	if (!sdl_window)
		return false;
	sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
	if (!sdl_renderer)
	{
		fprintf(stderr, "SDL_CreateRenderer failed; available renderers:\n");
		int num = SDL_GetNumRenderDrivers();
		for (int i = 0; i < num; ++i)
		{
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(i, &info);
			fprintf(stderr, " - %s\n", info.name);
		}
		return false;
	}
	SDL_RenderSetLogicalSize(sdl_renderer, VIDXRES, VIDYRES);
	SDL_RenderSetIntegerScale(sdl_renderer, forceIntegerScaling && fullscreen ? SDL_TRUE : SDL_FALSE);

	std::string filteringMode = "nearest";
	if (resizable)
	{
		if (pixelFilteringMode == 1)
			filteringMode = "linear";
		else if (pixelFilteringMode == 2)
			filteringMode = "best";
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, filteringMode.c_str());
	sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
		VIDXRES, VIDYRES);

	if (fullscreen)
		SDL_RaiseWindow(sdl_window);
	//Uncomment this to enable resizing
	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (!firstRun)
		LoadWindowPosition();

	return true;
}

void SDLBlit(pixel * vid)
{
	SDL_UpdateTexture(sdl_texture, NULL, vid, VIDXRES * sizeof (Uint32));
	// need to clear the renderer if there are black edges (fullscreen, or resizable window)
	if (fullscreen || resizable)
		SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
	SDL_RenderPresent(sdl_renderer);
}

unsigned int CalculateMousePosition(int *x, int *y)
{
	int globalMx, globalMy;
	unsigned int mouseState = SDL_GetGlobalMouseState(&globalMx, &globalMy);
	int windowX, windowY;
	SDL_GetWindowPosition(sdl_window, &windowX, &windowY);
	int sizeX, sizeY;
	SDL_GetWindowSize(sdl_window, &sizeX, &sizeY);

	float scaleX = (float)sizeX / VIDXRES;
	float scaleY = (float)sizeY / VIDYRES;
	bool rescaleX = scaleX > scaleY;
	// The size of the black borders SDL places on the edges
	int diffX = rescaleX ? static_cast<int>((sizeX - VIDXRES * scaleY) / 2) : 0;
	int diffY = !rescaleX ? static_cast<int>((sizeY - VIDYRES * scaleX) / 2) : 0;

	// Take position relative to where the black border starts, then rescale it to be TPT coordinates
	if (x)
		*x = (float)(globalMx - windowX - diffX) / (sizeX - diffX * 2) * VIDXRES;
	if (y)
		*y = (float)(globalMy - windowY - diffY) / (sizeY - diffY * 2) * VIDYRES;

	return mouseState;
}

int SDLGetModifiers()
{
	return SDL_GetModState();
}

bool calculatedInitialMouse = false;
bool hasMouseMoved = false;
bool doManualMouseCalculation = false;
bool capturingMouse = false;
Point lastMousePosition;
bool inOldInterface = false;

// When the mouse hasn't moved yet, sdl will always report (0, 0) as the position in events
// By using SDL_GetGlobalMouseState, we can get the real mouse position
// (should be accurate except for resizable windows / fullscreen)
void GetTempMousePosition(int *x, int *y)
{
	if (!hasMouseMoved)
		CalculateMousePosition(x, y);
}

int EventProcess(SDL_Event event, ui::Window * eventHandler)
{
	switch (event.type)
	{
	case SDL_QUIT:
		if (Engine::Ref().IsFastQuit())
			Engine::Ref().Shutdown();
		return 1;
	case SDL_KEYDOWN:
		if (SDL_GetModState() & KMOD_GUI)
			break;

		sdl_key = event.key.keysym.sym; // LEGACY
		sdl_mod = static_cast<unsigned short>(SDL_GetModState()); // LEGACY

		if (eventHandler)
			eventHandler->DoKeyPress(event.key.keysym.sym, event.key.keysym.scancode, event.key.repeat, event.key.keysym.mod&KMOD_SHIFT, event.key.keysym.mod&(KMOD_CTRL|KMOD_GUI), event.key.keysym.mod&KMOD_ALT);

		if (eventHandler && event.key.keysym.sym == SDLK_ESCAPE && eventHandler->CanQuit())
			return true;
		else if (event.key.keysym.sym == 'q' && (sdl_mod & (KMOD_CTRL | KMOD_GUI)))
		{
			bool wasConfirmed = false;
			// Two separate quit dialogs ... remove some day
			if (inOldInterface)
			{
				wasConfirmed = confirm_ui(vid_buf, "You are about to quit", "Are you sure you want to quit?", "Quit");
			}
			else
			{
				Engine::Ref().ShowWindow(new ConfirmPrompt([&](bool confirmed) {
					wasConfirmed = confirmed;
				}, "You are about to quit", "Are you sure you want to quit?", "Quit"));
				MainLoop(true);
			}
			if (wasConfirmed)
			{
				Engine::Ref().Shutdown();
				return 1;
			}
		}
		break;
	case SDL_KEYUP:
		if (SDL_GetModState() & KMOD_GUI)
			break;

		sdl_mod = static_cast<unsigned short>(SDL_GetModState()); // LEGACY

		if (eventHandler)
			eventHandler->DoKeyRelease(event.key.keysym.sym, event.key.keysym.scancode, event.key.repeat, event.key.keysym.mod&KMOD_SHIFT, event.key.keysym.mod&(KMOD_CTRL|KMOD_GUI), event.key.keysym.mod&KMOD_ALT);
		break;

	case SDL_TEXTINPUT:
		if (!Engine::Ref().DoTextInput() || SDL_GetModState() & KMOD_GUI)
			break;

		sdl_textinput = std::string(event.text.text);
		if (eventHandler)
			eventHandler->DoTextInput(event.text.text);

		break;
	case SDL_MOUSEWHEEL:
	{
		// int x = event.wheel.x;
		int y = event.wheel.y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
		{
			// x *= -1;
			y *= -1;
		}

		sdl_wheel = y; // LEGACY

		if (eventHandler)
			eventHandler->DoMouseWheel(lastMousePosition.X, lastMousePosition.Y, y);
		break;
	}

	case SDL_MOUSEBUTTONDOWN:
	{
		int mx = event.motion.x, my = event.motion.y;
		GetTempMousePosition(&mx, &my);
		if (mx < 0 || my < 0 || mx >= VIDXRES || my >= VIDYRES)
			break;
		if (eventHandler)
			eventHandler->DoMouseDown(mx, my, event.button.button);
		lastMousePosition = Point(mx, my);
		doManualMouseCalculation = true;
		break;
	}
	case SDL_MOUSEBUTTONUP:
	{
		int mx = event.motion.x, my = event.motion.y;
		GetTempMousePosition(&mx, &my);
		if (eventHandler)
			eventHandler->DoMouseUp(mx, my, event.button.button);
		lastMousePosition = Point(mx, my);
		doManualMouseCalculation = false;
		if (capturingMouse)
		{
			SDL_CaptureMouse(SDL_FALSE);
			capturingMouse = false;
		}
		break;
	}
	case SDL_MOUSEMOTION:
	{
		int mx = event.motion.x, my = event.motion.y;
		GetTempMousePosition(&mx, &my);
		if (eventHandler)
			eventHandler->DoMouseMove(mx, my, mx-lastMousePosition.X, my-lastMousePosition.Y);
		lastMousePosition = Point(mx, my);
		hasMouseMoved = true;
		break;
	}
	case SDL_WINDOWEVENT:
	{
		switch (event.window.event)
		{
		// Wait for the window to be shown, then try to calculate the initial mouse position
		// This is only necessary to have the cursor show in the correct position instead of (0, 0)
		// Because all mouse events will call CalculateMousePosition anyway
		case SDL_WINDOWEVENT_SHOWN:
			if (!calculatedInitialMouse)
			{
				int mousex, mousey;
				CalculateMousePosition(&mousex, &mousey);
				lastMousePosition = Point(mousex, mousey);
				if (eventHandler)
					eventHandler->DoMouseMove(mousex, mousey, 0, 0);
				calculatedInitialMouse = true;
			}
			break;
		}
		break;
	}
	case SDL_DROPFILE:
		eventHandler->DoFileDrop(event.drop.file);
		SDL_free(event.drop.file);
		break;
	}
	return 0;
}

uint32_t lastTick, drawingTimer;
void MainLoop(bool secondaryLoop)
{
	SDL_Event event;
	Engine &engine = Engine::Ref();
	bool isSecondaryEngineLoop = engine.IsSecondaryEngineLoop();
	if (secondaryLoop)
	{
		engine.ProcessWindowUpdates();
		engine.SetSecondaryEngineLoop(true);
	}
	int currentStackSize = engine.GetStackSize();
	while (engine.Running())
	{
		ui::Window * top = engine.GetTop();
		// weird thing done when entering old interfaces that use sdl_poll loops
		if (sendNewEvents)
		{
			int mx, my;
			SDL_GetMouseState(&mx, &my);
			mx /= engine.GetScale();
			my /= engine.GetScale();
			if (Point(mx, my) != lastMousePosition)
			{
				top->DoMouseMove(mx, my, mx-lastMousePosition.X, my-lastMousePosition.Y);
				lastMousePosition = Point(mx, my);
			}
			top->DoKeyRelease(0, 0, false, false, false, false);
			sendNewEvents = false;
		}
		top->UpdateComponents();

		sdl_textinput = "";
		sdl_key = sdl_wheel = 0;
		while (SDL_PollEvent(&event))
		{
			int ret = EventProcess(event, top);
			if (ret)
				engine.CloseTop();
		}
		if (doManualMouseCalculation && !fullscreen)
		{
			int outsideX, outsideY;
			CalculateMousePosition(&outsideX, &outsideY);
			if (outsideX < 0 || outsideX >= VIDXRES || outsideY < 0 || outsideY >= VIDYRES)
			{
#ifndef DEBUG
				if (!capturingMouse)
				{
					SDL_CaptureMouse(SDL_TRUE);
					capturingMouse = true;
				}
#else
				top->DoMouseMove(outsideX, outsideY, outsideX - lastMousePosition.X, outsideY - lastMousePosition.Y);
				lastMousePosition = Point(outsideX, outsideY);
#endif
			}
			else if (capturingMouse)
			{
				SDL_CaptureMouse(SDL_FALSE);
				capturingMouse = false;
			}
		}

		uint32_t currentTick = SDL_GetTicks();
		top->DoTick(currentTick-lastTick, false);
		drawingTimer += currentTick - lastTick;
		lastTick = currentTick;

		int drawcap = engine.GetDrawingFrequency();
		if (!drawcap || drawingTimer > 1000.f/drawcap)
		{
			drawingTimer = 0;
			top->DoDraw(vid_buf, Point(XRES+BARSIZE, YRES+MENUSIZE), top->GetPosition());
			SDLBlit(vid_buf);
		}
		limit_fps();

		engine.ProcessWindowUpdates();
		if (secondaryLoop && engine.GetStackSize() < currentStackSize)
			break;
		inOldInterface = false;
	}


	if (secondaryLoop)
	{
		engine.SetSecondaryEngineLoop(isSecondaryEngineLoop);
	}
}

void PushToClipboard(std::string text)
{
	SDL_SetClipboardText(text.c_str());
}

std::string PullFromClipboard()
{
	return std::string(SDL_GetClipboardText());
}

unsigned int GetTicks()
{
	return SDL_GetTicks();
}


void BlueScreen(const char * detailMessage)
{
	//std::string errorDetails = "Details: " + std::string(detailMessage);
	SDL_Event event;
	const char * errorHelp = "An unrecoverable fault has occurred, please report this to jacob1:\n"
		" https://powdertoy.co.uk/Discussions/Thread/View.html?Thread=11117\n"
		" OR the built in bug reporter.\n\n"
		"Note: TPT will now restart and reload your work";
	int positionX = (XRES+BARSIZE)/2-textwidth(errorHelp)/2-50, positionY = (YRES+MENUSIZE)/2-100;

	fillrect(vid_buf, -1, -1, XRES+BARSIZE+1, YRES+MENUSIZE+1, 17, 114, 169, 210);

	drawtext(vid_buf, positionX, positionY, "ERROR", 255, 255, 255, 255);
	drawtext(vid_buf, positionX, positionY + 14, detailMessage, 255, 255, 255, 255);
	drawtext(vid_buf, positionX, positionY  + 28, errorHelp, 255, 255, 255, 255);

	pixel* vid_buf2 = (pixel*)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	memcpy(vid_buf2, vid_buf, (XRES+BARSIZE)*(YRES+MENUSIZE)*PIXELSIZE);
	std::vector<Point> food;
	std::list<Point> tron;
	unsigned int tronSize = 5, score = 0;
	int tronDirection = 2;
	char scoreString[20];
	bool gameRunning = false, gameLost = false;
	for (int i = 0; i < 10; i++)
		food.push_back(Point(rand()%(XRES+BARSIZE-6)+3, rand()%(YRES+MENUSIZE-6)+3));
	for (unsigned int i = 0; i < tronSize; i++)
		tron.push_back(Point(20, 10+i));

	//Death loop
	while(1)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				Platform::DoRestart(true, true);
			}
			else if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_UP:
					if (tronDirection != 2)
						tronDirection = 0;
					break;
				case SDLK_RIGHT:
					if (tronDirection != 3)
						tronDirection = 1;
					break;
				case SDLK_DOWN:
					if (tronDirection != 0)
						tronDirection = 2;
					break;
				case SDLK_LEFT:
					if (tronDirection != 1)
						tronDirection = 3;
					break;
				default:
#ifdef ANDROID
					Platform::DoRestart(true, true);
#endif
					break;
				}
				gameRunning = true;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				int mx = event.motion.x, my = event.motion.y;
				if (my < (YRES+MENUSIZE)/4)
				{
					if (tronDirection != 2)
						tronDirection = 0;
				}
				else if (my > YRES+MENUSIZE-(YRES+MENUSIZE)/4)
				{
					if (tronDirection != 0)
						tronDirection = 2;
				}
				else if (mx < (XRES+BARSIZE)/4)
				{
					if (tronDirection != 1)
						tronDirection = 3;
				}
				else if (mx > XRES+BARSIZE-(XRES+BARSIZE)/4)
				{
					if (tronDirection != 3)
						tronDirection = 1;
				}
				gameRunning = true;
			}
		}
		if (gameRunning && !gameLost)
		{
			Point tronHead = ((Point)tron.back());
			tronHead.X -= (tronDirection-2)%2;
			tronHead.Y += (tronDirection-1)%2;
			if (tronHead.X > 1 && tronHead.X < XRES+BARSIZE-2 && tronHead.Y > 1 && tronHead.Y < YRES+MENUSIZE-2)
				tron.push_back(tronHead);
			else
				gameLost = true;

			sprintf(scoreString, "Score: %i", score);
			drawtext(vid_buf, XRES-BARSIZE-50, 10, scoreString, 255, 255, 255, 255);
			for (std::vector<Point>::iterator iter = food.begin(); iter != food.end(); ++iter)
			{
				Point moo = (Point)*iter;
				vid_buf[moo.X + moo.Y*(XRES+BARSIZE)] = PIXRGB(255, 0, 0);
				vid_buf[moo.X + moo.Y*(XRES+BARSIZE)+1] = PIXRGB(100, 0, 0);
				vid_buf[moo.X + moo.Y*(XRES+BARSIZE)-1] = PIXRGB(100, 0, 0);
				vid_buf[moo.X + (moo.Y+1)*(XRES+BARSIZE)] = PIXRGB(100, 0, 0);
				vid_buf[moo.X + (moo.Y-1)*(XRES+BARSIZE)] = PIXRGB(100, 0, 0);
				if (tronHead.X >= moo.X - 3 && tronHead.X <= moo.X + 3 && tronHead.Y >= moo.Y - 3 && tronHead.Y <= moo.Y + 3)
				{
					food.erase(iter, iter+1);
					food.push_back(Point(rand()%(XRES+BARSIZE-6)+3, rand()%(YRES+MENUSIZE-6)+3));
					food.push_back(Point(rand()%(XRES+BARSIZE-6)+3, rand()%(YRES+MENUSIZE-6)+3));
					tronSize += 20;
					score++;
					break;
				}
			}
			if (!(rand()%20))
				tronSize++;
			int i = tronSize;
			for (std::list<Point>::iterator iter = tron.begin(); iter != tron.end(); ++iter)
			{
				Point point = *iter;
				vid_buf[point.X + point.Y*(XRES+BARSIZE)] = PIXRGB(0, 255-(int)(200.0f/tronSize*i), 0);
				i--;
				if (iter != --tron.end() && point == tronHead)
					gameLost = true;
			}
			while (tron.size() > tronSize)
				tron.pop_front();
		}
		SDLBlit(vid_buf);
		if (gameRunning && !gameLost)
			memcpy(vid_buf, vid_buf2, (XRES+BARSIZE)*(YRES+MENUSIZE)*PIXELSIZE);
		SDL_Delay(5);
	}
}

/*
 * Legacy functions only beyond this point
 */

int FPSwait = 0;
int SDLPoll()
{
	SDL_Event event;
	if (Engine::Ref().IsShutdown())
		return 1;

	if (!inOldInterface)
	{
		inOldInterface = true;
		the_game->OnDefocus();
	}
	sdl_textinput = "";
	sdl_key = sdl_wheel = 0;
	while (SDL_PollEvent(&event))
	{
		int ret = EventProcess(event, nullptr);
		if (ret)
			return ret;
	}

	sdl_mod = static_cast<unsigned short>(SDL_GetModState());
	limit_fps();
	sendNewEvents = true;
	return 0;
}

int pastFPS = 0;
float FPSB2 = 60.0f;
double frameTimeAvg = 0.0, correctedFrameTimeAvg = 60.0;
void limit_fps()
{
	int frameTime = SDL_GetTicks() - currentTime;

	frameTimeAvg = frameTimeAvg * .8 + frameTime * .2;
	float limitFPS = Engine::Ref().GetFpsLimit();
	if (limitFPS > 2)
	{
		double offset = 1000.0 / limitFPS - frameTimeAvg;
		if (offset > 0)
			//millisleep((Uint32)(offset + 0.5));
			SDL_Delay((Uint32)(offset + 0.5));
	}

	int correctedFrameTime = SDL_GetTicks() - currentTime;
	correctedFrameTimeAvg = correctedFrameTimeAvg * 0.95 + correctedFrameTime * 0.05;
	elapsedTime = currentTime-pastFPS;
	if (elapsedTime >= 200)
	{
		if (!FPSwait)
			FPSB2 = 1000.0f/(float)correctedFrameTimeAvg;
		if (FPSwait > 0)
			FPSwait--;
		pastFPS = currentTime;
	}
	currentTime = SDL_GetTicks();
}

void sdl_blit(int unused1, int unused2, int unused3, int unused4, pixel *vid, int unused5)
{
	SDLBlit(vid);
}

int mouse_get_state(int *x, int *y)
{
	//return (int)CalculateMousePosition(x, y);
	*x = lastMousePosition.X;
	*y = lastMousePosition.Y;
	//if (*x < 0 || *y < 0 || *x >= VIDXRES || *y >= VIDYRES)
	//	return 0;
	return SDL_GetMouseState(nullptr, nullptr);
}

int sdl_poll()
{
	Engine::Ref().StartTextInput();
	return SDLPoll();
}
#endif
