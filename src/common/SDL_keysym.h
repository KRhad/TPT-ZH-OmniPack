#ifndef SDL1_2
#ifdef SDL_R_INCL
#include <SDL_keycode.h>
#else
#include <SDL2/SDL_keycode.h>
#endif

#else // SDL1_2

#ifdef SDL_R_INCL
#include <SDL_keysym.h>
#else
#include <SDL/SDL_keysym.h>
#endif

#define KMOD_GUI KMOD_META
#define KMOD_LGUI KMOD_LMETA
#define KMOD_RGUI KMOD_RMETA
#define SDLK_LGUI SDLK_LMETA
#define SDLK_RGUI SDLK_RMETA

// This is COMPLETELY wrong, but I'm just sending keycode twice in both keycode/scancode arguments for Android port
#define SDL_SCANCODE_A 'a'
#define SDL_SCANCODE_B 'b'
#define SDL_SCANCODE_C 'c'
#define SDL_SCANCODE_D 'd'
#define SDL_SCANCODE_E 'e'
#define SDL_SCANCODE_F 'f'
#define SDL_SCANCODE_G 'g'
#define SDL_SCANCODE_H 'h'
#define SDL_SCANCODE_I 'i'
#define SDL_SCANCODE_K 'k'
#define SDL_SCANCODE_L 'l'
#define SDL_SCANCODE_N 'n'
#define SDL_SCANCODE_O 'o'
#define SDL_SCANCODE_P 'p'
#define SDL_SCANCODE_Q 'q'
#define SDL_SCANCODE_R 'r'
#define SDL_SCANCODE_S 's'
#define SDL_SCANCODE_T 't'
#define SDL_SCANCODE_U 'u'
#define SDL_SCANCODE_V 'v'
#define SDL_SCANCODE_W 'w'
#define SDL_SCANCODE_X 'x'
#define SDL_SCANCODE_Y 'y'
#define SDL_SCANCODE_Z 'z'
#define SDL_SCANCODE_0 '0'
#define SDL_SCANCODE_1 '1'
#define SDL_SCANCODE_2 '2'
#define SDL_SCANCODE_3 '3'
#define SDL_SCANCODE_4 '4'
#define SDL_SCANCODE_5 '5'
#define SDL_SCANCODE_6 '6'
#define SDL_SCANCODE_7 '7'
#define SDL_SCANCODE_8 '8'
#define SDL_SCANCODE_9 '9'
#define SDL_SCANCODE_ESCAPE SDLK_ESCAPE
#define SDL_SCANCODE_F1 SDLK_F1
#define SDL_SCANCODE_F2 SDLK_F2
#define SDL_SCANCODE_F3 SDLK_F3
#define SDL_SCANCODE_F5 SDLK_F5
#define SDL_SCANCODE_GRAVE SDLK_BACKQUOTE
#define SDL_SCANCODE_EQUALS SDLK_EQUALS
#define SDL_SCANCODE_BACKSPACE SDLK_BACKSPACE
#define SDL_SCANCODE_INSERT SDLK_INSERT
#define SDL_SCANCODE_TAB SDLK_TAB
#define SDL_SCANCODE_LEFTBRACKET SDLK_LEFTBRACKET
#define SDL_SCANCODE_RIGHTBRACKET SDLK_RIGHTBRACKET
#define SDL_SCANCODE_DELETE SDLK_DELETE
#define SDL_SCANCODE_SEMICOLON SDLK_SEMICOLON
#define SDL_SCANCODE_SPACE SDLK_SPACE

#endif
