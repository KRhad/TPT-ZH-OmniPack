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

class Simulation;
extern Simulation * luaSim;

extern pixel *lua_vid_buf;
extern std::deque<std::pair<std::string, int>> logHistory;
extern unsigned long loop_time;

class LuaSmartRef;
extern int *lua_el_mode;
extern LuaSmartRef *lua_el_func, *lua_gr_func;
extern std::vector<LuaSmartRef> lua_el_func_v, lua_gr_func_v;
extern std::vector<LuaSmartRef> luaCtypeDrawHandlers, luaCreateHandlers, luaCreateAllowedHandlers, luaChangeTypeHandlers;
extern LuaSmartRef *tptPart;

void luacon_open();
void luacon_openmultiplayer();
void luacon_openscriptmanager();
void luacon_openeventcompat();
void initLegacyProps();
void luaopen_multiplayer(lua_State *l);
void luaopen_scriptmanager(lua_State *l);
void luaopen_eventcompat(lua_State *l);
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
int luacon_partsread(lua_State* l);
int luacon_partswrite(lua_State* l);
int luacon_partread(lua_State* l);
int luacon_partwrite(lua_State* l);
int luacon_elementread(lua_State* l);
int luacon_elementwrite(lua_State* l);
int luacon_transitionread(lua_State* l);
int luacon_transitionwrite(lua_State* l);
int luacon_tptIndex(lua_State *l);
int luacon_tptNewIndex(lua_State *l);
int process_command_lua(pixel *vid_buf, const char *command, std::string *result);
void lua_hook(lua_State *L, lua_Debug *ar);

extern int getPartIndex_curIdx;

//TPT Interface
int luatpt_getelement(lua_State *l);
int luatpt_drawtext(lua_State* l);
int luatpt_create(lua_State* l);
int luatpt_setpause(lua_State* l);
int luatpt_togglepause(lua_State* l);
int luatpt_togglewater(lua_State* l);
int luatpt_setconsole(lua_State* l);
int luatpt_log(lua_State* l);
int luatpt_set_pressure(lua_State* l);
int luatpt_set_aheat(lua_State* l);
int luatpt_set_velocity(lua_State* l);
int luatpt_set_gravity(lua_State* l);
int luatpt_reset_gravity_field(lua_State* l);
int luatpt_reset_velocity(lua_State* l);
int luatpt_reset_spark(lua_State* l);
int luatpt_set_property(lua_State* l);
int luatpt_get_property(lua_State* l);
int luatpt_drawpixel(lua_State* l);
int luatpt_drawrect(lua_State* l);
int luatpt_fillrect(lua_State* l);
int luatpt_drawcircle(lua_State* l);
int luatpt_fillcircle(lua_State* l);
int luatpt_drawline(lua_State* l);
int luatpt_textwidth(lua_State* l);
int luatpt_get_name(lua_State* l);
int luatpt_delete(lua_State* l);
int luatpt_input(lua_State* l);
int luatpt_message_box(lua_State* l);
int luatpt_confirm(lua_State* l);
int luatpt_get_numOfParts(lua_State* l);
int luatpt_start_getPartIndex(lua_State* l);
int luatpt_getPartIndex(lua_State* l);
int luatpt_next_getPartIndex(lua_State* l);
int luatpt_hud(lua_State* l);
int luatpt_gravity(lua_State* l);
int luatpt_airheat(lua_State* l);
int luatpt_active_menu(lua_State* l);
int luatpt_menu_enabled(lua_State* l);
int luatpt_menu_click(lua_State* l);
int luatpt_num_menus(lua_State* l);
int luatpt_tab_menu(lua_State* l);
int luatpt_decorations_enable(lua_State* l);
int luatpt_cmode_set(lua_State* l);
int luatpt_error(lua_State* l);
int luatpt_heat(lua_State* l);
int luatpt_setfire(lua_State* l);
int luatpt_setdebug(lua_State* l);
int luatpt_setfpscap(lua_State* l);
int luatpt_setdrawcap(lua_State* l);
int luatpt_getscript(lua_State* l);
int luatpt_setwindowsize(lua_State* l);
int luatpt_screenshot(lua_State* l);
int luatpt_record(lua_State* l);
int luatpt_getclip(lua_State* l);
int luatpt_setclip(lua_State* l);
int luatpt_bubble(lua_State* l);
int luatpt_maxframes(lua_State* l);
int luatpt_getwall(lua_State* l);
int luatpt_createwall(lua_State* l);
int luatpt_set_elecmap(lua_State* l);
int luatpt_get_elecmap(lua_State* l);
int luatpt_perfectCircle(lua_State* l);
int luatpt_indestructible(lua_State* l);
int luatpt_oldmenu(lua_State* l);

void set_map(int x, int y, int width, int height, float value, int map);

extern char* LuaCode;
extern int LuaCodeLen;
extern bool ranLuaCode;
void ReadLuaCode();
void ExecuteEmbededLuaCode();
#else
#include "lua/LuaEvents.h"
#endif

bool HandleEvent(LuaEvents::EventTypes eventType, Event * event);
#endif
