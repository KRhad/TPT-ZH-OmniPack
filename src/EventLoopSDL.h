#ifndef EVENTLOOPSDL_H
#define EVENTLOOPSDL_H

#include <string>
#include "graphics/Pixel.h"

// legacy / should be deleted variables
extern int savedWindowX;
extern int savedWindowY;

// legacy variables
extern unsigned short sdl_mod;
extern std::string sdl_textinput;
extern int sdl_key, sdl_wheel;

void StartTextInput();
void StopTextInput();
void SaveWindowPosition();

void SDLInit();
int SDLOpen();
void SDLSetScreen(bool resizable_, int pixelFilteringMode_, bool fullscreen_, bool altFullscreen_,
				  bool forceIntegerResolution, bool canRecreateWindow);

void SDLBlit(pixel * vid);

unsigned int CalculateMousePosition(int *x, int *y);
int SDLGetModifiers();



union SDL_Event;
namespace ui
{
	class Window;
}
int EventProcess(SDL_Event event, ui::Window * eventHandler);
int SDLPoll();
void MainLoop(bool secondaryLoop = false);
void limit_fps();


void PushToClipboard(std::string text);
std::string PullFromClipboard();

unsigned int GetTicks();

void BlueScreen(const char * detailMessage);


// legacy functions
void sdl_blit(int unused1, int unused2, int unused3, int unused4, pixel * vid, int unused5);
int mouse_get_state(int *x, int *y);
int sdl_poll();

#endif
