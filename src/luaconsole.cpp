/**
 * Powder Toy - Lua console
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

#ifdef LUACONSOLE
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

#if defined(LIN) || defined(MACOSX)
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifdef WIN
#include <direct.h>
#endif

#include "legacy_console.h"
#include "defines.h"
#include "EventLoopSDL.h"
#include "graphics.h"
#include "interface.h"
#include "luaconsole.h"
#include "luascriptinterface.h"
#include "powder.h"

#include "common/Format.h"
#include "common/Platform.h"
#include "common/tpt-rand.h"
#include "interface/Engine.h"
#include "game/Request.h"
#include "graphics/Renderer.h"
#include "gui/dialogs/ConfirmPrompt.h"
#include "gui/dialogs/ErrorPrompt.h"
#include "gui/game/PowderToy.h"
#include "lua/LuaComponent.h"
#include "lua/LuaSmartRef.h"
#include "simulation/Simulation.h"

#include "simulation/elements/ANIM.h"

Simulation * luaSim;
int *lua_el_mode;
LuaSmartRef *lua_el_func, *lua_gr_func;
std::vector<LuaSmartRef> lua_el_func_v, lua_gr_func_v;
std::vector<LuaSmartRef> luaCtypeDrawHandlers, luaCreateHandlers, luaCreateAllowedHandlers, luaChangeTypeHandlers;
std::deque<std::pair<std::string, int>> logHistory;
lua_State *l;
int tptProperties; //Table for some TPT properties
int tptPropertiesVersion;
int tptElements; //Table for TPT element names
int tptParts, tptPartsMeta, tptElementTransitions, tptPartsCData, tptPartMeta, cIndex;
LuaSmartRef *tptPart = nullptr;

static int mathRandom(lua_State *l)
{
	// only thing that matters is that the rng not be luacon_sim->rng when !inSimEvent
	double lower, upper;
	switch (lua_gettop(l))
	{
	case 0:
		lua_pushnumber(l, RNG::Ref().uniform01Double());
		return 1;

	case 1:
		lower = 1;
		upper = luaL_checknumber(l, 1);
		break;

	default:
		lower = luaL_checknumber(l, 1);
		upper = luaL_checknumber(l, 2);
		break;
	}
	if (upper < lower)
	{
		luaL_error(l, "interval is empty");
	}
	if (lower >= INT32_MIN && upper <= INT32_MAX)
	{
		int il = int(lower);
		int iu = int(upper);
		if (((unsigned int)(iu) - (unsigned int)(il) + 1U)) // the exact expression the RNG divides something by
		{
			lua_pushinteger(l, RNG::Ref().between(il, iu));
		}
		else
		{
			lua_pushinteger(l, int(RNG::Ref()()));
		}
	}
	else if (lower >= UINT32_C(0) && upper <= UINT32_MAX)
	{
		lua_pushnumber(l, RNG::Ref()());
	}
	else
	{
		lua_pushnumber(l, lower + RNG::Ref().uniform01Double() * (upper - lower));
	}
	return 1;
}

static int mathRandomseed(lua_State *l)
{
	RNG::Ref().seed(luaL_checkinteger(l, 1));
	return 0;
}

void luacon_open()
{
	const static struct luaL_Reg tptluaapi [] = {
		{"log", &tpt_log},
		{"getUserName", tpt_getUserName},
		{"getscript",&tpt_getscript},
		{"installScriptManager", tpt_installScriptManager},
		{"screenshot", tpt_screenshot},
		{"record", tpt_record},
		{"debug", tpt_debug},
		{"fpsCap", tpt_fpsCap},
		{"drawCap", tpt_drawCap},
		{"tab_menu", tpt_tab_menu},
		{"bubble",&tpt_bubble},
#ifndef NOMOD
		{"maxframes",&tpt_maxframes},
#endif
		{"indestructible",&tpt_indestructible},
		{"oldmenu", &tpt_oldmenu},
		{NULL,NULL}
	};

	l = luaL_newstate();
	tpt_lua_setmainthread(l);
	luaL_openlibs(l);
	luaopen_bit(l);
	luaL_register(l, "tpt", tptluaapi);
	
	luaSim = globalSim;
	initSimulationAPI(l);
	initRendererAPI(l);
	initFileSystemAPI(l);
	initInterfaceAPI(l);
	initGraphicsAPI(l);
	initElementsAPI(l);
	initToolsAPI(l);
	initPlatformAPI(l);
	initEventAPI(l);
	initHttpAPI(l);
	initSocketAPI(l);
	initBZ2API(l);

	lua_getglobal(l, "math");
	lua_pushcfunction(l, mathRandom);
	lua_setfield(l, -2, "random");
	lua_pushcfunction(l, mathRandomseed);
	lua_setfield(l, -2, "randomseed");
	lua_pop(l, 1);

	lua_getglobal(l, "tpt");

	tptProperties = lua_gettop(l);

	lua_newtable(l);
	tptPropertiesVersion = lua_gettop(l);
	lua_pushinteger(l, SAVE_VERSION);
	lua_setfield(l, tptPropertiesVersion, "major");
	lua_pushinteger(l, MINOR_VERSION);
	lua_setfield(l, tptPropertiesVersion, "minor");
	lua_pushinteger(l, BUILD_NUM);
	lua_setfield(l, tptPropertiesVersion, "build");
	// I'm not expecting any forks of this mod to release that change major/minor version,
	//  so upstream version is the same as major/minor version
	lua_pushinteger(l, SAVE_VERSION);
	lua_setfield(l, tptPropertiesVersion, "upstreamMajor");
	lua_pushinteger(l, MINOR_VERSION);
	lua_setfield(l, tptPropertiesVersion, "upstreamMinor");
	lua_pushinteger(l, BUILD_NUM);
	lua_setfield(l, tptPropertiesVersion, "upstreamBuild");
	lua_pushboolean(l, false);
	lua_setfield(l, tptPropertiesVersion, "snapshot");
	lua_pushboolean(l, false);
	lua_setfield(l, tptPropertiesVersion, "beta");
#ifdef ANDROID
	lua_pushinteger(l, MOBILE_MAJOR);
	lua_setfield(l, tptPropertiesVersion, "mobilemajor");
	lua_pushinteger(l, MOBILE_MINOR);
	lua_setfield(l, tptPropertiesVersion, "mobileminor");
	lua_pushinteger(l, MOBILE_BUILD);
	lua_setfield(l, tptPropertiesVersion, "mobilebuild");
#endif
	lua_pushinteger(l, MOD_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod");
	lua_pushinteger(l, MOD_MINOR_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod_minor");
	lua_pushinteger(l, MOD_SAVE_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod_save");
	lua_pushinteger(l, MOD_BUILD_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod_build");
	lua_setfield(l, tptProperties, "version");

	SETCONST(l, DEBUG_PARTS);
	SETCONST(l, DEBUG_ELEMENTPOP);
	SETCONST(l, DEBUG_LINES);
	SETCONST(l, DEBUG_PARTICLE);
	SETCONST(l, DEBUG_SURFNORM);
	SETCONST(l, DEBUG_SIMHUD);
	SETCONST(l, DEBUG_RENHUD);

	lua_gr_func_v = std::vector<LuaSmartRef>(PT_NUM);
	lua_gr_func = &lua_gr_func_v[0];
	lua_el_func_v = std::vector<LuaSmartRef>(PT_NUM);
	lua_el_func = &lua_el_func_v[0];
	lua_el_mode = new int[PT_NUM];
	std::fill(lua_el_mode, lua_el_mode + PT_NUM, 0);
	luaCtypeDrawHandlers = std::vector<LuaSmartRef>(PT_NUM);
	luaCreateHandlers = std::vector<LuaSmartRef>(PT_NUM);
	luaCreateAllowedHandlers = std::vector<LuaSmartRef>(PT_NUM);
	luaChangeTypeHandlers = std::vector<LuaSmartRef>(PT_NUM);

	lua_sethook(l, &lua_hook, LUA_MASKCOUNT, 4000000);
}

void luacon_openmultiplayer()
{
#ifndef TOUCHUI
	luaopen_multiplayer(l);
#endif
}

void luacon_openscriptmanager()
{
#ifndef TOUCHUI
	luaopen_scriptmanager(l);
#endif
}

void luacon_opencompat()
{
	luaopen_compat(l);
}

void luacon_openstickmancontrol()
{
#ifdef TOUCHUI
	luaopen_stickmancontrol(l);
#endif
}

void luacon_step(int mx, int my)
{
	TickEvent ev = TickEvent();
	HandleEvent(LuaEvents::tick, &ev);
}

int luaL_tostring(lua_State *L, int n)
{
	luaL_checkany(L, n);
	switch (lua_type(L, n))
	{
		case LUA_TNUMBER:
			tpt_lua_pushString(L, tpt_lua_toString(L, n));
			break;
		case LUA_TSTRING:
			lua_pushvalue(L, n);
			break;
		case LUA_TBOOLEAN:
			tpt_lua_pushString(L, (lua_toboolean(L, n) ? "true" : "false"));
			break;
		case LUA_TNIL:
			lua_pushliteral(L, "nil");
			break;
		default:
			lua_pushfstring(L, "%s: %p", luaL_typename(L, n), lua_topointer(L, n));
			break;
	}
	return 1;
}

void luacon_log(std::string log)
{
	if (logHistory.size() >= 20)
		logHistory.pop_back();

	logHistory.push_front(std::pair<std::string, int>(log, 150));
	std::cout << log << std::endl;
}

std::string lastCode;

// logs from tpt.logs() and print()
std::string logs;
bool hasLogs = false;

int luacon_eval(const char *command, std::string *result)
{
	int level = lua_gettop(l), ret = -1;
	std::string text;
	bool hasText = false;
	logs = "";
	hasLogs = false;
	if (lastCode.length())
	{
		lastCode = lastCode + "\n" + command;
	}
	else
	{
		lastCode = command;
	}
	std::string returnTest = "return " + lastCode;
	luaL_loadbuffer(l, returnTest.c_str(), returnTest.length(), "@console");
	if (lua_type(l, -1) != LUA_TFUNCTION)
	{
		lua_pop(l, 1);
		luaL_loadbuffer(l, lastCode.c_str(), lastCode.length(), "@console");
	}
	if (lua_type(l, -1) != LUA_TFUNCTION)
	{
		std::string err = luacon_geterror();
		*result = err;
		if (err.find("near '<eof>'") != err.npos)
		{
			*result = "...";
		}
		else
		{
			lastCode = "";
		}
		return 0;
	}
	else
	{
		lastCode = "";
		ret = tpt_lua_pcall(l, 0, LUA_MULTRET, 0);
		if (ret)
			return ret;
		else
		{
			for (level++; level <= lua_gettop(l); level++)
			{
				luaL_tostring(l, level);
				std::string retVal = tpt_lua_optString(l, -1, "");
				if (hasText)
				{
					text = text + ", " + retVal;
				}
				else
				{
					text = retVal;
					hasText = true;
				}
				lua_pop(l, 1);
			}
			if (hasLogs)
			{
				if (hasText)
				{
					text = logs + "; " + text;
				}
				else
				{
					text = logs;
					hasText = true;
				}
				logs = "";
				hasLogs = false;
			}
			if (hasText)
			{
				if (result->length())
				{
					*result = *result + "; " + text;
				}
				else
				{
					*result = text;
				}
			}
		}
	}
	return ret;
}

void lua_hook(lua_State *L, lua_Debug *ar)
{
	if (ar->event == LUA_HOOKCOUNT && int(Platform::GetTime() - luaExecutionStart) > luaHookTimeout)
	{
		bool wasConfirmed = false;
		auto prompt = new ConfirmPrompt("Infinite Loop", "The Lua code might have an infinite loop. Press OK to stop it", "OK");
		prompt->SetCallback({ [&wasConfirmed](bool confirmed) {
			wasConfirmed = confirmed;
		} });
		Engine::Ref().ShowWindow(prompt);
		MainLoop(true);

		if (!wasConfirmed)
			return;
		luaL_error(l,"Error: Infinite loop");
		luaExecutionStart = Platform::GetTime();
	}
}

int luaUpdateWrapper(UPDATE_FUNC_ARGS)
{
	auto *builtinUpdate = luaSim->origElements[parts[i].type].Update;
	if (builtinUpdate && lua_el_mode[parts[i].type] == UPDATE_AFTER)
	{
		if (builtinUpdate(UPDATE_FUNC_SUBCALL_ARGS))
			return 1;
		x = (int)(parts[i].x+0.5f);
		y = (int)(parts[i].y+0.5f);
	}

	if (lua_el_func[parts[i].type])
	{
		int retval = 0, callret;
		lua_rawgeti(l, LUA_REGISTRYINDEX, lua_el_func[parts[i].type]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, x);
		lua_pushinteger(l, y);
		lua_pushinteger(l, surround_space);
		lua_pushinteger(l, nt);
		callret = tpt_lua_pcall(l, 5, 1, 0);
		if (callret)
			luacon_log(luacon_geterror());
		if(lua_isboolean(l, -1)){
			retval = lua_toboolean(l, -1);
		}
		lua_pop(l, 1);
		if (retval)
		{
			return 1;
		}
		x = (int)(parts[i].x+0.5f);
		y = (int)(parts[i].y+0.5f);
	}
	if (builtinUpdate && lua_el_mode[parts[i].type] == UPDATE_BEFORE)
	{
		if (builtinUpdate(UPDATE_FUNC_SUBCALL_ARGS))
			return 1;
		x = (int)(parts[i].x+0.5f);
		y = (int)(parts[i].y+0.5f);
	}
	return 0;
}

int luaGraphicsWrapper(GRAPHICS_FUNC_ARGS)
{
	if (lua_gr_func[cpart->type])
	{
		int cache = 0, callret;
		int i = cpart - parts; // pointer arithmetic be like
		lua_rawgeti(l, LUA_REGISTRYINDEX, lua_gr_func[cpart->type]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, *colr);
		lua_pushinteger(l, *colg);
		lua_pushinteger(l, *colb);
		callret = tpt_lua_pcall(l, 4, 10, 0, eventTraitSimGraphics);
		if (callret)
		{
			luacon_log(luacon_geterror());
			lua_pop(l, 1);
		}
		else
		{
			bool valid = true;
			for (int i = -10; i < 0; i++)
				if (!lua_isnumber(l, i) && !lua_isnil(l, i))
				{
					valid = false;
					break;
				}
			if (valid)
			{
				cache = luaL_optint(l, -10, 0);
				*pixel_mode = luaL_optint(l, -9, *pixel_mode);
				*cola = luaL_optint(l, -8, *cola);
				*colr = luaL_optint(l, -7, *colr);
				*colg = luaL_optint(l, -6, *colg);
				*colb = luaL_optint(l, -5, *colb);
				*firea = luaL_optint(l, -4, *firea);
				*firer = luaL_optint(l, -3, *firer);
				*fireg = luaL_optint(l, -2, *fireg);
				*fireb = luaL_optint(l, -1, *fireb);
			}
			lua_pop(l, 10);
		}
		return cache;
	}
	return 0;
}

bool luaCtypeDrawWrapper(CTYPEDRAW_FUNC_ARGS)
{
	bool ret = false;
	if (luaCtypeDrawHandlers[sim->parts[i].type])
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, luaCtypeDrawHandlers[sim->parts[i].type]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, t);
		lua_pushinteger(l, v);
		if (tpt_lua_pcall(l, 3, 1, 0))
		{
			luacon_log("In ctype draw: " + luacon_geterror());
			lua_pop(l, 1);
		}
		else
		{
			if (lua_isboolean(l, -1))
				ret = lua_toboolean(l, -1);
			lua_pop(l, 1);
		}
	}
	return ret;
}

void luaCreateWrapper(ELEMENT_CREATE_FUNC_ARGS)
{
	if (luaCreateHandlers[sim->parts[i].type])
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, luaCreateHandlers[sim->parts[i].type]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, x);
		lua_pushinteger(l, y);
		lua_pushinteger(l, t);
		lua_pushinteger(l, v);
		if (tpt_lua_pcall(l, 5, 0, 0))
		{
			luacon_log("In create func: " + luacon_geterror());
			lua_pop(l, 1);
		}
	}
}

bool luaCreateAllowedWrapper(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	bool ret = false;
	if (luaCreateAllowedHandlers[t])
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, luaCreateAllowedHandlers[t]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, x);
		lua_pushinteger(l, y);
		lua_pushinteger(l, t);
		if (tpt_lua_pcall(l, 4, 1, 0))
		{
			luacon_log("In create allowed: " + luacon_geterror());
			lua_pop(l, 1);
		}
		else
		{
			if (lua_isboolean(l, -1))
				ret = lua_toboolean(l, -1);
			lua_pop(l, 1);
		}
	}
	return ret;
}

void luaChangeTypeWrapper(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (luaChangeTypeHandlers[sim->parts[i].type])
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, luaChangeTypeHandlers[sim->parts[i].type]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, x);
		lua_pushinteger(l, y);
		lua_pushinteger(l, from);
		lua_pushinteger(l, to);
		if (tpt_lua_pcall(l, 5, 0, 0))
		{
			luacon_log("In change type: " + luacon_geterror());
			lua_pop(l, 1);
		}
	}
}

std::string luacon_geterror()
{
	luaL_tostring(l, -1);
	std::string err = tpt_lua_optString(l, -1, "failed to execute");
	lua_pop(l, 1);
	return err;
}

void luacon_close()
{
	delete tptPart;
	delete[] lua_el_mode;
	lua_el_func_v.clear();
	lua_gr_func_v.clear();
	luaCtypeDrawHandlers.clear();
	luaCreateHandlers.clear();
	luaCreateAllowedHandlers.clear();
	luaChangeTypeHandlers.clear();
	for (auto &component_and_ref : grabbed_components)
	{
		the_game->RemoveComponent(component_and_ref.first->GetComponent());
		component_and_ref.second.Clear();
		component_and_ref.first->owner_ref = component_and_ref.second;
	}
	lua_close(l);
	if (LuaCode)
		free(LuaCode);
}

int process_command_lua(pixel *vid_buf, const char *command, std::string *result)
{
	if (command && strlen(command))
	{
		if (strncmp(command, "!", 1)==0)
		{
			return process_command_old(luaSim, vid_buf, command+1, result);
		}
		else
		{
			int commandret = luacon_eval(command, result);
			if (commandret)
			{
				std::string err = luacon_geterror();
				if (!console_mode)
					luacon_log(err);
				*result = mystrdup(err.c_str());
			}
		}
	}
	return 1;
}

int tpt_log(lua_State* l)
{
	std::string buffer;
	bool hasBuffer = false;
	int args = lua_gettop(l), i;
	for (i = 1; i <= args; i++)
	{
		luaL_tostring(l, -1);
		std::string logVal = tpt_lua_optString(l, -1, "");
		if (hasBuffer)
		{
			buffer = logVal + ", " + buffer;
		}
		else
		{
			buffer = logVal;
			hasBuffer = true;
		}
		lua_pop(l, 2);
	}

	if (console_mode)
	{
		if (hasLogs)
		{
			logs += "; " + buffer;
		}
		else
		{
			logs = buffer;
			hasLogs = true;
		}
		return 0;
	}
	else
	{
		luacon_log(buffer);
		return 0;
	}
}

int tpt_getUserName(lua_State* l)
{
	if (svf_login)
	{
		tpt_lua_pushString(l, svf_user);
		return 1;
	}
	tpt_lua_pushString(l, "");
	return 1;
}

int tpt_getscript(lua_State* l)
{
	int scriptID = luaL_checkinteger(l, 1);
	std::string filename = tpt_lua_checkString(l, 2);
	int runScript = luaL_optint(l, 3, 0);
	int confirmPrompt = luaL_optint(l, 4, 1);

	return getScriptInner(l, scriptID, filename, runScript, confirmPrompt);
}

int tpt_installScriptManager(lua_State *l)
{
	return getScriptInner(l, 1, "autorun.lua", 1, 0);
}

int getScriptInner(lua_State *l, int scriptID, std::string filename, int runScript, int confirmPrompt)
{
	std::stringstream url;
	url << "https://starcatcher.us/scripts/main.lua?get=" << scriptID;
	if (confirmPrompt)
	{
		bool wasConfirmed = false;
		auto prompt = new ConfirmPrompt("Do you want to install script?", url.str().c_str(), "Install");
		prompt->SetCallback({ [&wasConfirmed](bool confirmed) {
			wasConfirmed = confirmed;
		} });
		Engine::Ref().ShowWindow(prompt);
		MainLoop(true);
		if (!wasConfirmed)
			return 0;
	}

	int ret;
	std::string scriptData = Request::Simple(url.str(), &ret);
	if (scriptData.empty())
	{
		return luaL_error(l, "Server did not return data");
	}
	if (ret != 200)
	{
		return luaL_error(l, Request::GetStatusCodeDesc(ret).c_str());
	}

	if (scriptData.find("Invalid script ID") != scriptData.npos)
	{
		return luaL_error(l, "Invalid Script ID");
	}

	FILE *outputfile = fopen(filename.c_str(), "r");
	if (outputfile)
	{
		fclose(outputfile);
		outputfile = NULL;
		if (confirmPrompt)
		{
			bool wasConfirmed = false;
			auto prompt = new ConfirmPrompt("File already exists, overwrite?", filename.c_str(), "Overwrite");
			prompt->SetCallback({ [&wasConfirmed](bool confirmed) {
				wasConfirmed = confirmed;
			} });
			Engine::Ref().ShowWindow(prompt);
			MainLoop(true);
			if (!wasConfirmed)
				return 0;
		}
	}

	outputfile = fopen(filename.c_str(), "wb");
	if (!outputfile)
	{
		return luaL_error(l, "Unable to write to file");
	}

	fputs(scriptData.c_str(), outputfile);
	fclose(outputfile);
	outputfile = NULL;
	if (runScript)
	{
		std::stringstream luaCommand;
		luaCommand << "dofile('" << filename << "')";
		tpt_lua_dostring(l, luaCommand.str());
	}

	return 0;
}

int tpt_screenshot(lua_State* l)
{
	int captureUI = luaL_optint(l, 1, 0);
	int fileType = luaL_optint(l, 2, 0);
	if (fileType < 0 || fileType > 2)
		return luaL_error(l, "Invalid screenshot format");
	std::string filename = Renderer::Ref().TakeScreenshot(captureUI, fileType);
	tpt_lua_pushString(l, filename);
	return 1;
}

int tpt_record(lua_State* l)
{
	if (!lua_isboolean(l, -1))
		return luaL_typerror(l, 1, lua_typename(l, LUA_TBOOLEAN));
	bool record = lua_toboolean(l, -1);
	if (!record)
	{
		Renderer::Ref().StopRecording();
		return 0;
	}
	int recordingFolder = Renderer::Ref().StartRecording();
	lua_pushinteger(l, recordingFolder);
	return 1;
}

int tpt_debug(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushinteger(l, debug_flags);
		return 1;
	}
	int debugFlags = luaL_checkint(l, 1);
	debug_flags = debugFlags;
	return 0;
}

int tpt_fpsCap(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, Engine::Ref().GetFpsLimit());
		return 1;
	}
	float fpscap = luaL_checknumber(l, 1);
	if (fpscap < 2.0f)
		return luaL_error(l, "fps cap too small");
	Engine::Ref().SetFpsLimit(fpscap);
	return 0;
}

int tpt_drawCap(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, Engine::Ref().GetDrawingFrequency());
		return 1;
	}
	int drawcap = luaL_checkint(l, 1);
	if (drawcap < 0)
		return luaL_error(l, "draw cap too small");
	Engine::Ref().SetDrawingFrequency(drawcap);
	return 0;
}

int tpt_tab_menu(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount > 0)
	{
		luaL_checktype(l, 1, LUA_TBOOLEAN);
		show_tabs = lua_toboolean(l, 1);
	}
	lua_pushinteger(l, show_tabs);
	return 1;
}

int tpt_bubble(lua_State* l)
{
	int x = luaL_optint(l, 1, 0);
	int y = luaL_optint(l, 1, 0);
	int first, rem1, rem2, i;

	first = luaSim->part_create(-1, x+18, y, PT_SOAP);
	rem1 = first;

	for (i = 1; i<=30; i++)
	{
		rem2 = luaSim->part_create(-1, (int)(x+18*cosf(i/5.0f)), (int)(y+18*sinf(i/5.0f)), PT_SOAP);

		if (rem1 != -1 && rem2 != -1)
		{
			parts[rem1].ctype = 7;
			parts[rem1].tmp = rem2;
			parts[rem2].tmp2 = rem1;
		}

		rem1 = rem2;
	}

	if (rem1 != -1 && first != -1)
	{
		parts[rem1].ctype = 7;
		parts[rem1].tmp = first;
		parts[first].tmp2 = rem1;
		parts[first].ctype = 7;
	}
	return 0;
}

#ifndef NOMOD
int tpt_maxframes(lua_State* l)
{
	int maxFrames = luaL_optint(l,1,-1);
	if (maxFrames == -1)
	{
		lua_pushnumber(l, static_cast<ANIM_ElementDataContainer&>(*luaSim->elementData[PT_ANIM]).GetMaxFrames());
		return 1;
	}
	if (maxFrames > 0 && maxFrames <= 256)
		static_cast<ANIM_ElementDataContainer&>(*luaSim->elementData[PT_ANIM]).SetMaxFrames(maxFrames);
	else
		return luaL_error(l, "must be between 1 and 256");
	static_cast<ANIM_ElementDataContainer&>(*luaSim->elementData[PT_ANIM]).Simulation_Cleared(luaSim);
	return 0;
}
#endif

int tpt_indestructible(lua_State* l)
{
	int el = 0, ind;
	if(lua_isnumber(l, 1))
	{
		el = luaL_optint(l, 1, 0);
		if (el<0 || el>=PT_NUM)
			return luaL_error(l, "Unrecognised element number '%d'", el);
	}
	else
	{
		std::string name = tpt_lua_optString(l, 1, "dust");
		if (!console_parse_type(name.c_str(), &el, NULL, luaSim))
			return luaL_error(l, "Unrecognised element '%s'", name.c_str());
	}
	ind = luaL_optint(l, 2, 1);
	if (ind)
	{
		luaSim->elements[el].Properties |= PROP_INDESTRUCTIBLE;
	}
	else
	{
		luaSim->elements[el].Properties &= ~PROP_INDESTRUCTIBLE;
	}
	return 0;
}

int tpt_oldmenu(lua_State *l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, old_menu);
		return 1;
	}
#ifdef TOUCHUI
	return luaL_error(l, "Old menu not supported when using the touch interface");
#else
	int oldmenu = luaL_checkint(l, 1);
	old_menu = oldmenu;
	return 0;
#endif
}

char* LuaCode = NULL;
int LuaCodeLen = 0;
bool ranLuaCode = true;
void ReadLuaCode()
{
	if (!Platform::FileExists("luacode.txt"))
	{
		Engine::Ref().ShowWindow(new ErrorPrompt("Place some code in luacode.txt"));
		return;
	}
	char* code = (char*)file_load("luacode.txt", &LuaCodeLen);
	if (!code)
	{
		Engine::Ref().ShowWindow(new ErrorPrompt("Error reading luacode.txt"));
		return;
	}
	if (LuaCode)
	{
		free(LuaCode);
		LuaCode = NULL;
	}
	// lua bytecode starts with byte 27, don't allow since can't be read and can do strange things
	if (code[0] == '\x1b')
	{
		Engine::Ref().ShowWindow(new ErrorPrompt("Lua bytecode detected"));
		return;
	}
	LuaCode = code;
	ranLuaCode = false;
}

void ConfirmRunEmbeddedLuaCode()
{
	if (!ranLuaCode && LuaCode)
	{
		FILE* previewCode = fopen("newluacode.txt", "w");
		ranLuaCode = true;
		if (!previewCode)
		{
			Engine::Ref().ShowWindow(new ErrorPrompt("Could not write code to newluacode.txt"));
			return;
		}
		fwrite(LuaCode, LuaCodeLen, 1, previewCode);
		fclose(previewCode);

		auto prompt = new ConfirmPrompt("Lua Code", "Run the lua code in newluacode.txt?", "Run");
		prompt->SetCallback({ [](bool confirmed) {
			if (confirmed)
				RunEmbeddedLuaCode();
		} });
		Engine::Ref().ShowWindow(prompt);
	}
}

void RunEmbeddedLuaCode()
{
	// lua bytecode starts with byte 27, don't allow since can't be read and can do strange things
	if (LuaCode[0] == '\x1b')
	{
		Engine::Ref().ShowWindow(new ErrorPrompt("Lua bytecode detected"));
		free(LuaCode);
		LuaCode = NULL;
		return;
	}

	//whitelist of functions we allow. Hopefully safe but just in case we write it to newluacode.txt above and ask the user to check it
	if (luaL_dostring(l,"\n\
		env = {\n\
			print = print,\n\
			ipairs = ipairs,\n\
			next = next,\n\
			pairs = pairs,\n\
			pcall = pcall,\n\
			tonumber = tonumber,\n\
			tostring = tostring,\n\
			type = type,\n\
			unpack = unpack,\n\
			coroutine = { create = coroutine.create, resume = coroutine.resume, \n\
				running = coroutine.running, status = coroutine.status, \n\
				wrap = coroutine.wrap }, \n\
			string = { byte = string.byte, char = string.char, find = string.find, \n\
				format = string.format, gmatch = string.gmatch, gsub = string.gsub, \n\
				len = string.len, lower = string.lower, match = string.match, \n\
				rep = string.rep, reverse = string.reverse, sub = string.sub, \n\
				upper = string.upper },\n\
			table = { insert = table.insert, maxn = table.maxn, remove = table.remove, \n\
				sort = table.sort },\n\
			math = { abs = math.abs, acos = math.acos, asin = math.asin, \n\
				atan = math.atan, atan2 = math.atan2, ceil = math.ceil, cos = math.cos, \n\
				cosh = math.cosh, deg = math.deg, exp = math.exp, floor = math.floor, \n\
				fmod = math.fmod, frexp = math.frexp, huge = math.huge, \n\
				ldexp = math.ldexp, log = math.log, log10 = math.log10, max = math.max, \n\
				min = math.min, modf = math.modf, pi = math.pi, pow = math.pow, \n\
				rad = math.rad, random = math.random, randomseed = math.randomseed, sin = math.sin, sinh = math.sinh, \n\
				sqrt = math.sqrt, tan = math.tan, tanh = math.tanh },\n\
			os = { clock = os.clock, difftime = os.difftime, time = os.time, date = os.date, exit = os.exit },\n\
			tpt = tpt,\n\
			sim = sim, simulation = simulation,\n\
			elem = elem, elements = elements,\n\
			gfx = gfx, graphics = graphics,\n\
			ren = ren, renderer = renderer,\n\
			bit = bit,\n\
			socket = { gettime = socket.gettime }} --[[I think socket.gettime() is safe?]]\n\
			\n\
		"))
	{
		luacon_log(luacon_geterror()); //if large above thing errored
	}

	luaExecutionStart = Platform::GetTime();
#if LUA_VERSION_NUM >= 502
	if (luaL_dostring(l, "local code = loadfile(\"newluacode.txt\", nil, env) if code then code() end"))
#else
	if (luaL_dostring(l, "local code = loadfile(\"newluacode.txt\") if code then setfenv(code, env) code() end"))
#endif
	{
		luacon_log(luacon_geterror());
	}
}

#else
#include "lua/LuaEvents.h"
#endif

/**
* Handles an event
*
* @param eventType Value from the EventTypes enum
* @param event The event object
* @return false if event canceled
*/
bool HandleEvent(LuaEvents::EventTypes eventType, Event * event)
{
	EventTraits eventTrait = eventTraitNone;
	if (eventType == LuaEvents::aftersimdraw || eventType == LuaEvents::beforesimdraw)
		eventTrait = eventTraitSimGraphics;
#ifdef LUACONSOLE
	return LuaEvents::HandleEvent(l, event, "tptevents-" + Format::NumberToString<int>(eventType), eventTrait);
#else
	return true;
#endif
}
