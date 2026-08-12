#pragma once

#ifndef TPT_SDL3
# define TPT_SDL3 0
#endif

// SDL3 deliberately reports removed SDL2 names as compile errors unless the
// official transition aliases are enabled. 1.0.10 keeps those aliases only for
// stable key/scancode constants while platform-facing APIs are migrated behind
// explicit TPT_SDL3 branches.
#if TPT_SDL3
# define SDL_ENABLE_OLD_NAMES
# include <SDL3/SDL.h>

// SDL3 retired these SDL2 media/application scancodes. They remain part of
// TPT's public Lua constant table, so retain their established numeric ABI.
# define SDL_SCANCODE_WWW             SDL_Scancode(264)
# define SDL_SCANCODE_MAIL            SDL_Scancode(265)
# define SDL_SCANCODE_CALCULATOR      SDL_Scancode(266)
# define SDL_SCANCODE_COMPUTER        SDL_Scancode(267)
# define SDL_SCANCODE_BRIGHTNESSDOWN  SDL_Scancode(275)
# define SDL_SCANCODE_BRIGHTNESSUP    SDL_Scancode(276)
# define SDL_SCANCODE_DISPLAYSWITCH   SDL_Scancode(277)
# define SDL_SCANCODE_KBDILLUMTOGGLE  SDL_Scancode(278)
# define SDL_SCANCODE_KBDILLUMDOWN    SDL_Scancode(279)
# define SDL_SCANCODE_KBDILLUMUP      SDL_Scancode(280)
# define SDL_SCANCODE_APP1            SDL_Scancode(283)
# define SDL_SCANCODE_APP2            SDL_Scancode(284)
# define SDLK_WWW             SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_WWW))
# define SDLK_MAIL            SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MAIL))
# define SDLK_CALCULATOR      SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CALCULATOR))
# define SDLK_COMPUTER        SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COMPUTER))
# define SDLK_BRIGHTNESSDOWN  SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_BRIGHTNESSDOWN))
# define SDLK_BRIGHTNESSUP    SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_BRIGHTNESSUP))
# define SDLK_DISPLAYSWITCH   SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DISPLAYSWITCH))
# define SDLK_KBDILLUMTOGGLE  SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMTOGGLE))
# define SDLK_KBDILLUMDOWN    SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMDOWN))
# define SDLK_KBDILLUMUP      SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMUP))
# define SDLK_APP1            SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APP1))
# define SDLK_APP2            SDL_Keycode(SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APP2))
#else
# include <SDL.h>
#endif
