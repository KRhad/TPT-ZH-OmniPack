#include "PowderToySDL.h"
#include "SimulationConfig.h"
#include "WindowIcon.h"
#include "Config.h"
#include "gui/interface/Engine.h"
#include "graphics/Graphics.h"
#include "common/platform/Platform.h"
#include "common/clipboard/Clipboard.h"
#include "FrameSchedule.h"
#include <iostream>

int desktopWidth = 1280;
int desktopHeight = 1024;
SDL_Window *sdl_window = nullptr;
SDL_Renderer *sdl_renderer = nullptr;
SDL_Texture *sdl_texture = nullptr;
bool vsyncHint = false;
WindowFrameOps currentFrameOps;
bool momentumScroll = true;
bool showAvatars = true;
bool showLargeScreenDialog = false;
int mousex = 0;
int mousey = 0;
int mouseButton = 0;
bool mouseDown = false;
bool calculatedInitialMouse = false;
bool hasMouseMoved = false;
double correctedFrameTimeAvg = 0;
static bool prevContributesToFps = false;

static FrameSchedule tickSchedule;
static FrameSchedule drawSchedule;
static FrameSchedule clientTickSchedule;
static FrameSchedule fpsUpdateSchedule;

void StartTextInput()
{
#if TPT_SDL3
	SDL_StartTextInput(sdl_window);
#else
	SDL_StartTextInput();
#endif
}

void StopTextInput()
{
#if TPT_SDL3
	SDL_StopTextInput(sdl_window);
#else
	SDL_StopTextInput();
#endif
}

void SetTextInputRect(int x, int y, int w, int h)
{
	// Why does SDL_SetTextInputRect not take logical coordinates???
	SDL_Rect rect;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	#if TPT_SDL3
	float wx, wy, wwx, why;
	#else
	int wx, wy, wwx, why;
	#endif
	SDL_RenderLogicalToWindow(sdl_renderer, float(x), float(y), &wx, &wy);
	SDL_RenderLogicalToWindow(sdl_renderer, float(x + w), float(y + h), &wwx, &why);
	rect.x = int(wx);
	rect.y = int(wy);
	rect.w = int(wwx - wx);
	rect.h = int(why - wy);
#else
	// TODO: use SDL_RenderLogicalToWindow when ubuntu deigns to update to sdl 2.0.18
	auto scale = ui::Engine::Ref().windowFrameOps.scale;
	rect.x = x * scale;
	rect.y = y * scale;
	rect.w = w * scale;
	rect.h = h * scale;
#endif
#if TPT_SDL3
	SDL_SetTextInputArea(sdl_window, &rect, 0);
#else
	SDL_SetTextInputRect(&rect);
#endif
}

void ClipboardPush(ByteString text)
{
	SDL_SetClipboardText(text.c_str());
}

ByteString ClipboardPull()
{
	auto *text = SDL_GetClipboardText();
	ByteString result(text ? text : "");
	SDL_free(text);
	return result;
}

int GetModifiers()
{
	return SDL_GetModState();
}

unsigned int GetTicks()
{
	return SDL_GetTicks();
}

uint64_t GetNowNs()
{
	return uint64_t(SDL_GetTicks()) * UINT64_C(1'000'000);
}

static void CalculateMousePosition(int *x, int *y)
{
#if TPT_SDL3
	float globalMx, globalMy;
#else
	int globalMx, globalMy;
#endif
	SDL_GetGlobalMouseState(&globalMx, &globalMy);
	int windowX, windowY;
	SDL_GetWindowPosition(sdl_window, &windowX, &windowY);

	if (x)
		*x = (globalMx - windowX) / currentFrameOps.scale;
	if (y)
		*y = (globalMy - windowY) / currentFrameOps.scale;
}

void blit(pixel *vid)
{
	SDL_UpdateTexture(sdl_texture, nullptr, vid, WINDOWW * sizeof (Uint32));
	// need to clear the renderer if there are black edges (fullscreen, or resizable window)
	if (currentFrameOps.fullscreen || currentFrameOps.resizable)
		SDL_RenderClear(sdl_renderer);
#if TPT_SDL3
	SDL_RenderTexture(sdl_renderer, sdl_texture, nullptr, nullptr);
#else
	SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, nullptr);
#endif
	SDL_RenderPresent(sdl_renderer);
}

void UpdateRefreshRate()
{
	RefreshRate refreshRate;
#if TPT_SDL3
	auto display = SDL_GetDisplayForWindow(sdl_window);
	if (display)
	{
		if (auto *displayMode = SDL_GetCurrentDisplayMode(display); displayMode && displayMode->refresh_rate)
		{
			refreshRate = RefreshRateQueried{ int(displayMode->refresh_rate) };
		}
	}
#else
	int displayIndex = SDL_GetWindowDisplayIndex(sdl_window);
	if (displayIndex >= 0)
	{
		SDL_DisplayMode displayMode;
		if (!SDL_GetCurrentDisplayMode(displayIndex, &displayMode) && displayMode.refresh_rate)
		{
			refreshRate = RefreshRateQueried{ displayMode.refresh_rate };
		}
	}
#endif
	ui::Engine::Ref().SetRefreshRate(refreshRate);
}

void SDLOpen()
{
#if TPT_SDL3
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
#else
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
#endif
	{
		fprintf(stderr, "Initializing SDL (video subsystem): %s\n", SDL_GetError());
		Platform::Exit(-1);
	}
	Clipboard::Init();

	SDLSetScreen();

#if TPT_SDL3
	auto display = SDL_GetDisplayForWindow(sdl_window);
	if (display)
	{
		SDL_Rect rect;
		if (SDL_GetDisplayUsableBounds(display, &rect))
		{
			desktopWidth = rect.w;
			desktopHeight = rect.h;
		}
	}
#else
	int displayIndex = SDL_GetWindowDisplayIndex(sdl_window);
	if (displayIndex >= 0)
	{
		SDL_Rect rect;
		if (!SDL_GetDisplayUsableBounds(displayIndex, &rect))
		{
			desktopWidth = rect.w;
			desktopHeight = rect.h;
		}
	}
#endif
	UpdateRefreshRate();

	StopTextInput();
}

void SDLClose()
{
	if (SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_OPENGL)
	{
		// * nvidia-460 egl registers callbacks with x11 that end up being called
		//   after egl is unloaded unless we grab it here and release it after
		//   sdl closes the display. this is an nvidia driver weirdness but
		//   technically an sdl bug. glfw has this fixed:
		//   https://github.com/glfw/glfw/commit/9e6c0c747be838d1f3dc38c2924a47a42416c081
		SDL_GL_LoadLibrary(nullptr);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_GL_UnloadLibrary();
	}
	SDL_Quit();
}

void SDLSetScreen()
{
	auto newFrameOps = ui::Engine::Ref().windowFrameOps;
	auto newVsyncHint = false; // TODO: DrawLimitVsync
	if (FORCE_WINDOW_FRAME_OPS == forceWindowFrameOpsEmbedded)
	{
		newFrameOps.resizable = false;
		newFrameOps.fullscreen = false;
		newFrameOps.changeResolution = false;
		newFrameOps.forceIntegerScaling = false;
	}
	if (FORCE_WINDOW_FRAME_OPS == forceWindowFrameOpsHandheld)
	{
		newFrameOps.resizable = false;
		newFrameOps.fullscreen = true;
		newFrameOps.changeResolution = false;
		newFrameOps.forceIntegerScaling = false;
	}

	auto currentFrameOpsNorm = currentFrameOps.Normalize();
	auto newFrameOpsNorm = newFrameOps.Normalize();
	auto recreate = !sdl_window ||
	                // Recreate the window when toggling fullscreen, due to occasional issues
	                newFrameOpsNorm.fullscreen       != currentFrameOpsNorm.fullscreen       ||
	                // Also recreate it when enabling resizable windows, to fix bugs on windows,
	                //  see https://github.com/jacob1/The-Powder-Toy/issues/24
	                newFrameOpsNorm.resizable        != currentFrameOpsNorm.resizable        ||
	                newFrameOpsNorm.changeResolution != currentFrameOpsNorm.changeResolution ||
	                newFrameOpsNorm.blurryScaling    != currentFrameOpsNorm.blurryScaling    ||
	                newVsyncHint != vsyncHint;

	if (!(recreate ||
	      newFrameOpsNorm.scale               != currentFrameOpsNorm.scale               ||
	      newFrameOpsNorm.forceIntegerScaling != currentFrameOpsNorm.forceIntegerScaling))
	{
		return;
	}

	auto size = WINDOW * newFrameOpsNorm.scale;
	if (sdl_window && newFrameOpsNorm.resizable)
	{
		SDL_GetWindowSize(sdl_window, &size.X, &size.Y);
	}

	if (recreate)
	{
		if (sdl_texture)
		{
			SDL_DestroyTexture(sdl_texture);
			sdl_texture = nullptr;
		}
		if (sdl_renderer)
		{
			SDL_DestroyRenderer(sdl_renderer);
			sdl_renderer = nullptr;
		}
		if (sdl_window)
		{
			SaveWindowPosition();
			SDL_DestroyWindow(sdl_window);
			sdl_window = nullptr;
		}

#if TPT_SDL3
		SDL_WindowFlags flags = 0;
#else
		unsigned int flags = 0;
		unsigned int rendererFlags = 0;
#endif
		if (newFrameOpsNorm.fullscreen)
		{
#if TPT_SDL3
			// SDL3 configures exclusive versus desktop fullscreen after window
			// creation through SDL_SetWindowFullscreenMode().
#else
			flags = newFrameOpsNorm.changeResolution ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
		}
		if (newFrameOpsNorm.resizable)
		{
			flags |= SDL_WINDOW_RESIZABLE;
		}
#if !TPT_SDL3
		if (vsyncHint)
		{
			rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
		}
		#endif
#if TPT_SDL3
		sdl_window = SDL_CreateWindow(ByteString::Build(APPNAME, " ", RELEASE_LABEL).c_str(), size.X, size.Y, flags);
#else
		sdl_window = SDL_CreateWindow(ByteString::Build(APPNAME, " ", RELEASE_LABEL).c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.X, size.Y, flags);
#endif
		if (!sdl_window)
		{
			fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
			Platform::Exit(-1);
		}
#if TPT_SDL3
		if (newFrameOpsNorm.fullscreen)
		{
			if (newFrameOpsNorm.changeResolution)
			{
				SDL_DisplayMode closestMode;
				auto display = SDL_GetDisplayForWindow(sdl_window);
				if (!display || !SDL_GetClosestFullscreenDisplayMode(display, size.X, size.Y, 0.0f, false, &closestMode) ||
					!SDL_SetWindowFullscreenMode(sdl_window, &closestMode))
				{
					fprintf(stderr, "SDL3 exclusive fullscreen setup failed: %s\n", SDL_GetError());
					Platform::Exit(-1);
				}
			}
			else
			{
				SDL_SetWindowFullscreenMode(sdl_window, nullptr);
			}
			if (!SDL_SetWindowFullscreen(sdl_window, true))
			{
				fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
				Platform::Exit(-1);
			}
		}
#endif
		if constexpr (SET_WINDOW_ICON)
		{
			WindowIcon(sdl_window);
		}
		SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#if TPT_SDL3
		sdl_renderer = SDL_CreateRenderer(sdl_window, nullptr);
		if (sdl_renderer && vsyncHint)
			SDL_SetRenderVSync(sdl_renderer, 1);
#else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, newFrameOpsNorm.blurryScaling ? "linear" : "nearest");
		sdl_renderer = SDL_CreateRenderer(sdl_window, -1, rendererFlags);
#endif
		if (!sdl_renderer)
		{
			fprintf(stderr, "SDL_CreateRenderer failed; available renderers:\n");
			int num = SDL_GetNumRenderDrivers();
			for (int i = 0; i < num; ++i)
			{
#if TPT_SDL3
				fprintf(stderr, " - %s\n", SDL_GetRenderDriver(i));
#else
				SDL_RendererInfo info;
				SDL_GetRenderDriverInfo(i, &info);
				fprintf(stderr, " - %s\n", info.name);
#endif
			}
			Platform::Exit(-1);
		}
#if TPT_SDL3
		SDL_SetRenderLogicalPresentation(sdl_renderer, WINDOWW, WINDOWH,
			newFrameOpsNorm.forceIntegerScaling ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE : SDL_LOGICAL_PRESENTATION_LETTERBOX);
#else
		SDL_RenderSetLogicalSize(sdl_renderer, WINDOWW, WINDOWH);
#endif
		sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOWW, WINDOWH);
		if (!sdl_texture)
		{
			fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
			Platform::Exit(-1);
		}
#if TPT_SDL3
		SDL_SetTextureScaleMode(sdl_texture, newFrameOpsNorm.blurryScaling ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
#endif
		SDL_RaiseWindow(sdl_window);
		Clipboard::RecreateWindow();
	}
#if TPT_SDL3
	SDL_SetRenderLogicalPresentation(sdl_renderer, WINDOWW, WINDOWH,
		newFrameOpsNorm.forceIntegerScaling ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE : SDL_LOGICAL_PRESENTATION_LETTERBOX);
#else
	SDL_RenderSetIntegerScale(sdl_renderer, newFrameOpsNorm.forceIntegerScaling ? SDL_TRUE : SDL_FALSE);
#endif
	if (!(newFrameOpsNorm.resizable && SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_MAXIMIZED))
	{
		SDL_SetWindowSize(sdl_window, size.X, size.Y);
		LoadWindowPosition();
	}
	ApplyFpsLimit();
	if (newFrameOpsNorm.fullscreen)
	{
		SDL_RaiseWindow(sdl_window);
	}
	currentFrameOps = newFrameOps;
	vsyncHint = newVsyncHint;
}

static void EventProcess(const SDL_Event &sourceEvent)
{
	auto &engine = ui::Engine::Ref();
#if TPT_SDL3
	auto event = sourceEvent;
	if (!SDL_ConvertEventToRenderCoordinates(sdl_renderer, &event))
	{
		event = sourceEvent;
	}
#else
	auto &event = sourceEvent;
#endif
	switch (event.type)
	{
#if TPT_SDL3
	case SDL_EVENT_QUIT:
#else
	case SDL_QUIT:
#endif
		if (ALLOW_QUIT && (engine.GetFastQuit() || engine.CloseWindow()))
		{
			engine.Exit();
		}
		break;
#if TPT_SDL3
	case SDL_EVENT_KEY_DOWN:
#else
	case SDL_KEYDOWN:
#endif
		if (SDL_GetModState() & KMOD_GUI)
		{
			break;
		}
#if TPT_SDL3
		if (engine.GetGlobalQuit() && ALLOW_QUIT && !event.key.repeat && event.key.key == 'q' && (event.key.mod&KMOD_CTRL) && !(event.key.mod&KMOD_ALT))
			engine.ConfirmExit();
		else
			engine.onKeyPress(event.key.key, event.key.scancode, event.key.repeat, event.key.mod&KMOD_SHIFT, event.key.mod&KMOD_CTRL, event.key.mod&KMOD_ALT);
#else
		if (engine.GetGlobalQuit() && ALLOW_QUIT && !event.key.repeat && event.key.keysym.sym == 'q' && (event.key.keysym.mod&KMOD_CTRL) && !(event.key.keysym.mod&KMOD_ALT))
			engine.ConfirmExit();
		else
			engine.onKeyPress(event.key.keysym.sym, event.key.keysym.scancode, event.key.repeat, event.key.keysym.mod&KMOD_SHIFT, event.key.keysym.mod&KMOD_CTRL, event.key.keysym.mod&KMOD_ALT);
#endif
		break;
#if TPT_SDL3
	case SDL_EVENT_KEY_UP:
#else
	case SDL_KEYUP:
#endif
		if (SDL_GetModState() & KMOD_GUI)
		{
			break;
		}
#if TPT_SDL3
		engine.onKeyRelease(event.key.key, event.key.scancode, event.key.repeat, event.key.mod&KMOD_SHIFT, event.key.mod&KMOD_CTRL, event.key.mod&KMOD_ALT);
#else
		engine.onKeyRelease(event.key.keysym.sym, event.key.keysym.scancode, event.key.repeat, event.key.keysym.mod&KMOD_SHIFT, event.key.keysym.mod&KMOD_CTRL, event.key.keysym.mod&KMOD_ALT);
#endif
		break;
#if TPT_SDL3
	case SDL_EVENT_TEXT_INPUT:
#else
	case SDL_TEXTINPUT:
#endif
		if (SDL_GetModState() & KMOD_GUI)
		{
			break;
		}
		engine.onTextInput(ByteString(event.text.text).FromUtf8());
		break;
#if TPT_SDL3
	case SDL_EVENT_TEXT_EDITING:
#else
	case SDL_TEXTEDITING:
#endif
		if (SDL_GetModState() & KMOD_GUI)
		{
			break;
		}
		engine.onTextEditing(ByteString(event.edit.text).FromUtf8(), event.edit.start);
		break;
#if TPT_SDL3
	case SDL_EVENT_MOUSE_WHEEL:
#else
	case SDL_MOUSEWHEEL:
#endif
	{
		// int x = event.wheel.x;
		int y = event.wheel.y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
		{
			// x *= -1;
			y *= -1;
		}

		engine.onMouseWheel(mousex, mousey, y); // TODO: pass x?
		break;
	}
#if TPT_SDL3
	case SDL_EVENT_MOUSE_MOTION:
#else
	case SDL_MOUSEMOTION:
#endif
		mousex = event.motion.x;
		mousey = event.motion.y;
		engine.onMouseMove(mousex, mousey);

		hasMouseMoved = true;
		break;
#if TPT_SDL3
	case SDL_EVENT_DROP_FILE:
		engine.onFileDrop(event.drop.data);
#else
	case SDL_DROPFILE:
		engine.onFileDrop(event.drop.file);
		SDL_free(event.drop.file);
#endif
		break;
#if TPT_SDL3
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
#else
	case SDL_MOUSEBUTTONDOWN:
#endif
		// if mouse hasn't moved yet, sdl will send 0,0. We don't want that
		if (hasMouseMoved)
		{
			mousex = event.button.x;
			mousey = event.button.y;
		}
		mouseButton = event.button.button;
		engine.onMouseDown(mousex, mousey, mouseButton);

		mouseDown = true;
		if constexpr (!DEBUG)
		{
#if TPT_SDL3
			SDL_CaptureMouse(true);
#else
			SDL_CaptureMouse(SDL_TRUE);
#endif
		}
		break;
#if TPT_SDL3
	case SDL_EVENT_MOUSE_BUTTON_UP:
#else
	case SDL_MOUSEBUTTONUP:
#endif
		// if mouse hasn't moved yet, sdl will send 0,0. We don't want that
		if (hasMouseMoved)
		{
			mousex = event.button.x;
			mousey = event.button.y;
		}
		mouseButton = event.button.button;
		engine.onMouseUp(mousex, mousey, mouseButton);

		mouseDown = false;
		if constexpr (!DEBUG)
		{
#if TPT_SDL3
			SDL_CaptureMouse(false);
#else
			SDL_CaptureMouse(SDL_FALSE);
#endif
		}
		break;
#if TPT_SDL3
	case SDL_EVENT_WINDOW_SHOWN:
#else
	case SDL_WINDOWEVENT:
	{
		switch (event.window.event)
		{
		case SDL_WINDOWEVENT_SHOWN:
#endif
			if (!calculatedInitialMouse)
			{
				//initial mouse coords, sdl won't tell us this if mouse hasn't moved
				CalculateMousePosition(&mousex, &mousey);
				engine.initialMouse(mousex, mousey);
				engine.onMouseMove(mousex, mousey);
				calculatedInitialMouse = true;
			}
			break;
#if TPT_SDL3
	case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
#else
		case SDL_WINDOWEVENT_DISPLAY_CHANGED:
#endif
			UpdateRefreshRate();
			break;
#if !TPT_SDL3
		}
		break;
	}
#endif
	}
}

std::optional<uint64_t> EngineProcess()
{
	auto &engine = ui::Engine::Ref();

	{
		auto nowNs = GetNowNs();
		if (clientTickSchedule.HasElapsed(nowNs))
		{
			TickClient();
			clientTickSchedule.SetNow(nowNs);
		}
		clientTickSchedule.Arm(10);
		if (fpsUpdateSchedule.HasElapsed(nowNs))
		{
			engine.SetFps(1e9f / correctedFrameTimeAvg);
			fpsUpdateSchedule.SetNow(nowNs);
		}
		fpsUpdateSchedule.Arm(5);
	}

	if (showLargeScreenDialog)
	{
		showLargeScreenDialog = false;
		LargeScreenDialog();
	}

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		EventProcess(event);
	}

	std::optional<uint64_t> delay;
	auto nowNs = GetNowNs();
	auto effectiveDrawLimit = engine.GetEffectiveDrawCap();
	auto doDraw = !effectiveDrawLimit || drawSchedule.HasElapsed(nowNs);
	auto fpsLimit = ui::Engine::Ref().GetFpsLimit();
	auto doSimTick = true;
	if (std::holds_alternative<FpsLimitExplicit>(fpsLimit))
	{
		doSimTick = tickSchedule.HasElapsed(nowNs);
	}
	else if (std::holds_alternative<FpsLimitFollowDraw>(fpsLimit))
	{
		doSimTick = doDraw;
	}
	if (doDraw)
	{
		engine.Tick();
	}
	if (doSimTick)
	{
		auto thisContributesToFps = engine.GetContributesToFps();
		if (prevContributesToFps && thisContributesToFps)
		{
			auto correctedFrameTime = tickSchedule.GetFrameTime();
			correctedFrameTimeAvg = correctedFrameTimeAvg + (correctedFrameTime - correctedFrameTimeAvg) * 0.05;
		}
		prevContributesToFps = thisContributesToFps;
		engine.SimTick();
		tickSchedule.SetNow(nowNs);
	}
	if (doDraw)
	{
		engine.Draw();
		drawSchedule.SetNow(nowNs);
		SDLSetScreen();
		blit(engine.g->Data());
	}
	if (effectiveDrawLimit)
	{
		delay = drawSchedule.Arm(float(*effectiveDrawLimit)) / UINT64_C(1'000'000);
	}
	if (auto *fpsLimitExplicit = std::get_if<FpsLimitExplicit>(&fpsLimit))
	{
		auto simDelay = tickSchedule.Arm(fpsLimitExplicit->value) / UINT64_C(1'000'000);
		if (delay.has_value() && simDelay < *delay)
		{
			delay = simDelay;
		}
	}
	else if (std::holds_alternative<FpsLimitNone>(fpsLimit))
	{
		delay.reset();
	}
	return delay;
}
