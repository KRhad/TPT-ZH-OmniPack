#ifndef SDLCOMPAT_H
#define SDLCOMPAT_H

#ifndef SDL1_2

#ifdef SDL_R_INCL
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#if defined(WIN)
#ifdef SDL_R_INCL
#include <SDL_syswm.h>
#else
#include <SDL2/SDL_syswm.h>
#endif
#endif

#else // SDL1_2

#ifdef SDL_R_INCL
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif
#ifdef ANDROID
#include <SDL/SDL_screenkeyboard.h>
#endif

#endif // SDL1_2

#undef Above
#undef Below

#endif // SDLCOMPAT_H
