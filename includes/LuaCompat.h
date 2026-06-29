#ifndef LUAINC_H
#define LUAINC_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(LUA_R_INCL)
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#elif defined(LUA_COMPAT_ALL)
#include "lua5.2/lua.h"
#include "lua5.2/lauxlib.h"
#include "lua5.2/lualib.h"
#elif defined(LUAJIT)

#ifndef ANDROID
#include "luajit-2.0/lua.h"
#include "luajit-2.0/lauxlib.h"
#include "luajit-2.0/lualib.h"
#else
#include "luajit-2.1/lua.h"
#include "luajit-2.1/lauxlib.h"
#include "luajit-2.1/lualib.h"
#endif

#else
#include "lua5.1/lua.h"
#include "lua5.1/lauxlib.h"
#include "lua5.1/lualib.h"
#endif

LUALIB_API void tpt_lua_setmainthread(lua_State *L);
LUALIB_API void tpt_lua_getmainthread(lua_State *L);

#if LUA_VERSION_NUM >= 502
#define LUA_GLOBALSINDEX	LUA_RIDX_GLOBALS

LUALIB_API int (luaL_typerror) (lua_State *L, int narg, const char *tname);
#else
# ifndef lua_pushglobaltable // * Thank you moonjit
LUALIB_API void (lua_pushglobaltable) (lua_State *L);
# endif
#endif

#ifdef __cplusplus
}
#endif

#endif

