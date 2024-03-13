/**
 * Powder Toy - Lua console (header)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LUACONSOLEH
#define LUACONSOLEH
#ifdef LUACONSOLE

#include <deque>
#include <string>
#include <vector>

#include "LuaCompat.h"
#include "defines.h"
#include "graphics/Pixel.h"
#include "lua/LuaEvents.h"
#include "simulation/Element.h"

#define LUACON_MDOWN 1
#define LUACON_MUP 2
#define LUACON_MPRESS 3
#define LUACON_MUPALT 4
#define LUACON_MUPZOOM 5
#define LUACON_KDOWN 1
#define LUACON_KUP 2

//Bitmasks for things that might need recalculating after changes to tpt.el
#define LUACON_EL_MODIFIED_CANMOVE 0x1
#define LUACON_EL_MODIFIED_GRAPHICS 0x2
#define LUACON_EL_MODIFIED_MENUS 0x4

enum UpdateMode
{
	UPDATE_AFTER,
	UPDATE_REPLACE,
	UPDATE_BEFORE,
	NUM_UPDATEMODES,
};

class Simulation;
extern Simulation * luaSim;

extern std::deque<std::pair<std::string, int>> logHistory;

class LuaSmartRef;
extern int *lua_el_mode;
extern LuaSmartRef *lua_el_func, *lua_gr_func;
extern std::vector<LuaSmartRef> lua_el_func_v, lua_gr_func_v;
extern std::vector<LuaSmartRef> luaCtypeDrawHandlers, luaCreateHandlers, luaCreateAllowedHandlers, luaChangeTypeHandlers;
extern LuaSmartRef *tptPart;

void luacon_open();
void luacon_openmultiplayer();
void luacon_openscriptmanager();
void luacon_opencompat();
void luacon_openstickmancontrol();
void luaopen_multiplayer(lua_State *l);
void luaopen_scriptmanager(lua_State *l);
void luaopen_compat(lua_State *l);
void luaopen_stickmancontrol(lua_State *l);
int luaopen_bit(lua_State *L);
void luacon_step(int mx, int my);
void luacon_log(std::string log);
int luacon_eval(const char *command, std::string *result);
int luaUpdateWrapper(UPDATE_FUNC_ARGS);
int luaGraphicsWrapper(GRAPHICS_FUNC_ARGS);
bool luaCtypeDrawWrapper(CTYPEDRAW_FUNC_ARGS);
void luaCreateWrapper(ELEMENT_CREATE_FUNC_ARGS);
bool luaCreateAllowedWrapper(ELEMENT_CREATE_ALLOWED_FUNC_ARGS);
void luaChangeTypeWrapper(ELEMENT_CHANGETYPE_FUNC_ARGS);
std::string luacon_geterror();
void luacon_close();
int process_command_lua(pixel *vid_buf, const char *command, std::string *result);
void lua_hook(lua_State *L, lua_Debug *ar);

int tpt_log(lua_State *l);
int tpt_getUserName(lua_State *l);
int tpt_getscript(lua_State* l);
int tpt_installScriptManager(lua_State* l);
int getScriptInner(lua_State* l, int scriptID, std::string filename, int runScript, int confirmPrompt);
int tpt_screenshot(lua_State* l);
int tpt_record(lua_State* l);
int tpt_debug(lua_State* l);
int tpt_fpsCap(lua_State* l);
int tpt_drawCap(lua_State* l);
// Jacob1's Mod stuff
int tpt_tab_menu(lua_State* l);
int tpt_bubble(lua_State* l);
int tpt_maxframes(lua_State* l);
int tpt_indestructible(lua_State* l);
int tpt_oldmenu(lua_State* l);

extern char* LuaCode;
extern int LuaCodeLen;
extern bool ranLuaCode;
void ReadLuaCode();
void ConfirmRunEmbeddedLuaCode();
void RunEmbeddedLuaCode();
#else
#include "lua/LuaEvents.h"
#endif

bool HandleEvent(LuaEvents::EventTypes eventType, Event * event);
#endif
