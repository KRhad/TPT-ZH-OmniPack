/**
 * Powder Toy - miscellaneous functions
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

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <cmath>

#include "EventLoopSDL.h"
#include "misc.h"
#include "defines.h"
#include "interface.h"
#include "graphics.h"
#include "powder.h"
#include "hud.h"
#include "cJSON.h"
#include "luascriptinterface.h"

#include "common/Format.h"
#include "game/Brush.h"
#include "game/Favorite.h"
#include "game/Menus.h"
#include "graphics/Renderer.h"
#include "interface/Engine.h"
#include "simulation/Simulation.h"
#include "simulation/SnapshotHistory.h"
#include "simulation/Tool.h"

#include "gui/console/Console.h"
#include "simulation/elements/LIFE.h"

// tpt++ setting that isn't used but is preserved
bool nativeClipboard = false;

static char hex[] = "0123456789ABCDEF";

unsigned clamp_flt(float f, float min, float max)
{
	if (f<min)
		return 0;
	if (f>max)
		return 255;
	return (int)(255.0f*(f-min)/(max-min));
}

float restrict_flt(float f, float min, float max)
{
	// Fix crash in certain cases when f is nan
	if (!std::isfinite(f))
		return min;
	if (f < min)
		return min;
	if (f > max)
		return max;
	return f;
}

char *mystrdup(const char *s)
{
	char *x;
	if (s)
	{
		x = (char*)malloc(strlen(s)+1);
		strcpy(x, s);
		return x;
	}
	return NULL;
}

void strlist_add(struct strlist **list, char *str)
{
	struct strlist *item = (struct strlist *)malloc(sizeof(struct strlist));
	item->str = mystrdup(str);
	item->next = *list;
	*list = item;
}

int strlist_find(struct strlist **list, char *str)
{
	struct strlist *item;
	for (item=*list; item; item=item->next)
		if (!strcmp(item->str, str))
			return 1;
	return 0;
}

void strlist_free(struct strlist **list)
{
	struct strlist *item;
	while (*list)
	{
		item = *list;
		*list = (*list)->next;
		free(item);
	}
}

void clean_text(char *text, int vwidth)
{
	if (vwidth >= 0 && textwidth(text) > vwidth)
		text[textwidthx(text, vwidth)] = 0;	

	for (unsigned i = 0; i < strlen(text); i++)
		if (text[i] < ' ' || text[i] >= 127)
			text[i] = ' ';
}

void save_console_history(cJSON **historyArray, cJSON **resultsArray)
{
	for (auto end = consoleHistory.rend(), iter = consoleHistory.rbegin(); iter < end; iter++)
	{
		auto historyItem = *iter;
		cJSON_AddItemToArray(*historyArray, cJSON_CreateString(historyItem.first.c_str()));
		cJSON_AddItemToArray(*resultsArray, cJSON_CreateString(historyItem.second.c_str()));
	}
}

void cJSON_AddString(cJSON** obj, const char *name, int number)
{
	std::stringstream str;
	str << number;
	cJSON_AddStringToObject(*obj, name, str.str().c_str());
}

void setBool(cJSON *root, const char *name, bool flag)
{
	if (flag)
		cJSON_AddTrueToObject(root, name);
	else
		cJSON_AddFalseToObject(root, name);
}

bool doingUpdate = false;
void save_presets()
{
	char * outputdata;
	cJSON *root, *userobj, *versionobj, *recobj, *graphicsobj, *hudobj, *simulationobj, *consoleobj, *tmpobj;
	FILE *f = fopen("powder.pref", "wb");
	if(!f)
		return;
	root = cJSON_CreateObject();
	
	cJSON_AddStringToObject(root, "Powder Toy Preferences", "Don't modify this file unless you know what you're doing. P.S: editing the admin/mod fields in your user info doesn't give you magical powers");
	
	//Tpt++ User Info
	if (svf_login)
	{
		cJSON_AddItemToObject(root, "User", userobj=cJSON_CreateObject());
		cJSON_AddStringToObject(userobj, "Username", svf_user);
		cJSON_AddNumberToObject(userobj, "ID", atoi(svf_user_id));
		cJSON_AddStringToObject(userobj, "SessionID", svf_session_id);
		cJSON_AddStringToObject(userobj, "SessionKey", svf_session_key);
		if (svf_admin)
		{
			cJSON_AddStringToObject(userobj, "Elevation", "Admin");
		}
		else if (svf_mod)
		{
			cJSON_AddStringToObject(userobj, "Elevation", "Mod");
		}
		else {
			cJSON_AddStringToObject(userobj, "Elevation", "None");
		}
	}

	//Tpt++ Renderer settings
	cJSON_AddItemToObject(root, "Renderer", graphicsobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(graphicsobj, "ColourMode", Renderer::Ref().GetColorMode());
	cJSON_AddNumberToObject(graphicsobj, "DisplayMode", Renderer::Ref().GetDisplayMode());
	cJSON_AddNumberToObject(graphicsobj, "RenderMode", Renderer::Ref().GetRenderMode());
	setBool(graphicsobj, "DebugMode", DEBUG_MODE);
	setBool(graphicsobj, "GravityField", drawgrav_enable);
	setBool(graphicsobj, "Decorations", decorations_enable);
	cJSON_AddNumberToObject(graphicsobj, "TemperatureScale", globalSim->temperatureScale);

	//Tpt++ Simulation setting(s)
	cJSON_AddItemToObject(root, "Simulation", simulationobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(simulationobj, "EdgeMode", globalSim->edgeMode);
	cJSON_AddNumberToObject(simulationobj, "AmbientAirTemp", globalSim->air->GetAmbientAirTempPref());
	cJSON_AddNumberToObject(simulationobj, "NewtonianGravity", globalSim->grav->IsEnabled());
	cJSON_AddNumberToObject(simulationobj, "AmbientHeat", aheat_enable);
	cJSON_AddNumberToObject(simulationobj, "PrettyPowder", pretty_powder);
	cJSON_AddNumberToObject(simulationobj, "UndoHistoryLimit", SnapshotHistory::GetUndoHistoryLimit());
	setBool(simulationobj, "LoadPressure", globalSim->includePressure);
	cJSON_AddNumberToObject(simulationobj, "DecoSpace", globalSim->decoSpace);
	cJSON_AddNumberToObject(simulationobj, "RealisticHeat", realistic);

	//Tpt++ install check, prevents annoyingness
	cJSON_AddTrueToObject(root, "InstallCheck");

	//Tpt++ console history
	cJSON_AddItemToObject(root, "Console", consoleobj=cJSON_CreateObject());
	tmpobj = cJSON_CreateStringArray(NULL, 0);
	cJSON *resultsObj = cJSON_CreateStringArray(NULL, 0);
	save_console_history(&tmpobj, &resultsObj);
	cJSON_AddItemToObject(consoleobj, "History", tmpobj);
	cJSON_AddItemToObject(consoleobj, "HistoryResults", resultsObj);

	cJSON_AddItemToObject(root, "NativeClipboard", tmpobj=cJSON_CreateObject());
	setBool(tmpobj, "Enabled", nativeClipboard);

	//Version Info
	cJSON_AddItemToObject(root, "version", versionobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(versionobj, "major", SAVE_VERSION);
	cJSON_AddNumberToObject(versionobj, "minor", MINOR_VERSION);
	cJSON_AddNumberToObject(versionobj, "build", BUILD_NUM);
#ifndef NOMOD
	cJSON_AddNumberToObject(versionobj, "modmajor", MOD_VERSION);
	cJSON_AddNumberToObject(versionobj, "modminor", MOD_MINOR_VERSION);
	cJSON_AddNumberToObject(versionobj, "modbuild", MOD_BUILD_VERSION);
#endif
#ifdef ANDROID
	cJSON_AddNumberToObject(versionobj, "modmajor", MOBILE_MAJOR);
	cJSON_AddNumberToObject(versionobj, "modminor", MOBILE_MINOR);
	cJSON_AddNumberToObject(versionobj, "modbuild", MOBILE_BUILD);
#endif
	setBool(versionobj, "update", doingUpdate);
	setBool(versionobj, "updateChecks", doUpdates);

	cJSON * favArr = cJSON_CreateArray();
	std::vector<std::string> favorites = Favorite::Ref().BuildFavoritesList(true);
	for (std::vector<std::string>::iterator iter = favorites.begin(); iter != favorites.end(); ++iter)
		cJSON_AddItemToArray(favArr, cJSON_CreateString((*iter).c_str()));
	cJSON_AddItemToObject(root, "Favorites", favArr);

	//Fav Menu/Records
	cJSON_AddItemToObject(root, "records", recobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(recobj, "Total Time Played", ((double)currentTime/1000)+((double)totaltime/1000)-((double)totalafktime/1000)-((double)afktime/1000));
	cJSON_AddNumberToObject(recobj, "Average FPS", totalfps/frames);
	cJSON_AddNumberToObject(recobj, "Number of frames", frames);
	cJSON_AddNumberToObject(recobj, "Total AFK Time", ((double)totalafktime/1000)+((double)afktime/1000)+((double)prevafktime/1000));
	cJSON_AddNumberToObject(recobj, "Times Played", timesplayed);

	//HUDs
	cJSON_AddItemToObject(root, "HUD", hudobj=cJSON_CreateObject());
	cJSON_AddItemToObject(hudobj, "normal", cJSON_CreateIntArray(normalHud, HUD_OPTIONS));
	cJSON_AddItemToObject(hudobj, "debug", cJSON_CreateIntArray(debugHud, HUD_OPTIONS));

	//General settings
	cJSON_AddStringToObject(root, "Proxy", http_proxy_string);
	cJSON_AddNumberToObject(root, "Scale", Engine::Ref().GetScale());


	setBool(root, "Resizable", Engine::Ref().IsResizable());
	setBool(root, "ResizableMode", Engine::Ref().GetPixelFilteringMode());
	setBool(root, "Fullscreen", Engine::Ref().IsFullscreen());
	setBool(root, "AltFullscreen", Engine::Ref().IsAltFullscreen());
	setBool(root, "ForceIntegerScaling", Engine::Ref().IsForceIntegerScaling());
	setBool(root, "MomentumScroll", Engine::Ref().IsMomentumScroll());
	setBool(root, "FastQuit", Engine::Ref().IsFastQuit());
	setBool(root, "MouseClickRequired", stickyCategories);
	setBool(root, "PerfectCircleBrush", perfectCircleBrush);
	setBool(root, "GraveExitsConsole", graveExitsConsole);

	if (savedWindowX != INT_MAX)
		cJSON_AddNumberToObject(root, "WindowX", savedWindowX);
	if (savedWindowY != INT_MAX)
		cJSON_AddNumberToObject(root, "WindowY", savedWindowY);

	//additional settings from my mod
	cJSON_AddNumberToObject(root, "heatmode", heatmode);
	if (heatmode == 2)
	{
		cJSON_AddNumberToObject(root, "HeatModeLowerBound", lowesttemp);
		cJSON_AddNumberToObject(root, "HeatModeUpperBound", highesttemp);
	}
	cJSON_AddNumberToObject(root, "autosave", autosave);
	if (explUnlocked)
		cJSON_AddNumberToObject(root, "EXPL_unlocked", 1);
	if (old_menu)
		cJSON_AddNumberToObject(root, "old_menu", 1);
	if (finding & 0x8)
		cJSON_AddNumberToObject(root, "alt_find", 1);
	cJSON_AddNumberToObject(root, "dateformat", dateformat);
	cJSON_AddNumberToObject(root, "decobox_hidden", decobox_hidden);
	if (loadIncompatibleSaves)
		cJSON_AddNumberToObject(root, "loadIncompatibleSaves", 1);
	if (luaHookTimeout != 3000)
		cJSON_AddNumberToObject(root, "LuaHookTimeout", luaHookTimeout);

	cJSON_AddItemToObject(root, "SavePreview", tmpobj=cJSON_CreateObject());
	cJSON_AddNumberToObject(tmpobj, "scrollSpeed", scrollSpeed);
	cJSON_AddNumberToObject(tmpobj, "scrollSpeedMomentum", scrollSpeedMomentum);
	cJSON_AddNumberToObject(tmpobj, "scrollDeceleration", scrollDeceleration);
	cJSON_AddNumberToObject(tmpobj, "ShowIDs", show_ids);

	auto cachedName = static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).GetCachedName();
	auto cachedRuleString = static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).GetCachedRuleString();
	auto customGol = static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).GetCustomGOL();
	cJSON_AddItemToObject(root, "CustomGOL", tmpobj=cJSON_CreateObject());
	cJSON_AddStringToObject(tmpobj, "Name", cachedName.c_str());
	cJSON_AddStringToObject(tmpobj, "Rule", cachedRuleString.c_str());
	cJSON * customGolArr = cJSON_CreateArray();
	for (auto &cgol : customGol)
	{
		std::stringstream golStream;
		golStream << cgol.nameString << " " << cgol.ruleString << " " << cgol.color1 << " " << cgol.color2;
		cJSON_AddItemToArray(customGolArr, cJSON_CreateString(golStream.str().c_str()));
	}
	cJSON_AddItemToObject(tmpobj, "Types", customGolArr);


	outputdata = cJSON_Print(root);
	cJSON_Delete(root);

	fwrite(outputdata, 1, strlen(outputdata), f);
	fclose(f);
	free(outputdata);
}

/**
 * Loads console history from cJSON objects. resultObj may be null if powder.pref was created in vanilla tpt
 */
void load_console_history(cJSON *commandObj, cJSON *resultObj)
{
	consoleHistory.clear();

	int commandSize = cJSON_GetArraySize(commandObj);
	int resultSize = resultObj ? cJSON_GetArraySize(resultObj) : 0;

	for (int i = 0; i < commandSize; i++)
	{
		const char *command = cJSON_GetArrayItem(commandObj, i)->valuestring;
		const char *result;
		if (resultSize && i < resultSize)
			result = cJSON_GetArrayItem(resultObj, i)->valuestring;
		else
			result = "";
		consoleHistory.push_front({std::string(command), std::string(result)});
	}
}

int cJSON_GetInt(cJSON **tmpobj)
{
	const char* ret = (*tmpobj)->valuestring;
	if (ret)
		return atoi(ret);
	else
		return 0;
}

void load_presets(void)
{
	int prefdatasize = 0;
	char * prefdata = (char*)file_load("powder.pref", &prefdatasize);
	cJSON *root;
	if (prefdata && (root = cJSON_Parse(prefdata)))
	{
		cJSON *userobj, *versionobj, *recobj, *tmpobj, *graphicsobj, *hudobj, *simulationobj, *consoleobj, *itemobj, *customgolobj;
		
		//Read user data
		userobj = cJSON_GetObjectItem(root, "User");
		if (userobj && (tmpobj = cJSON_GetObjectItem(userobj, "SessionKey")))
		{
			svf_login = 1;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "Username")) && tmpobj->type == cJSON_String)
				strncpy(svf_user, tmpobj->valuestring, 63);
			else
				svf_user[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "ID")) && tmpobj->type == cJSON_Number)
				snprintf(svf_user_id, 63, "%i", tmpobj->valueint);
			else
				svf_user_id[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "SessionID")) && tmpobj->type == cJSON_String)
				strncpy(svf_session_id, tmpobj->valuestring, 63);
			else
				svf_session_id[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "SessionKey")) && tmpobj->type == cJSON_String)
				strncpy(svf_session_key, tmpobj->valuestring, 63);
			else
				svf_session_key[0] = 0;
			if ((tmpobj = cJSON_GetObjectItem(userobj, "Elevation")) && tmpobj->type == cJSON_String)
			{
				if (!strcmp(tmpobj->valuestring, "Admin"))
				{
					svf_admin = 1;
					svf_mod = 0;
				}
				else if (!strcmp(tmpobj->valuestring, "Mod"))
				{
					svf_mod = 1;
					svf_admin = 0;
				}
				else
				{
					svf_admin = 0;
					svf_mod = 0;
				}
			}
		}
		else 
		{
			svf_login = 0;
			svf_user[0] = 0;
			svf_user_id[0] = 0;
			svf_session_id[0] = 0;
			svf_admin = 0;
			svf_mod = 0;
		}
		
		//Read version data
		versionobj = cJSON_GetObjectItem(root, "version");
		if (versionobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(versionobj, "major")))
				last_major = (unsigned short)tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(versionobj, "minor")))
				last_minor = (unsigned short)tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(versionobj, "build")))
				last_build = (unsigned short)tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(versionobj, "modbuild")))
				last_modbuild = (unsigned short)tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(versionobj, "update")) && tmpobj->type == cJSON_True)
				update_flag = 1;
			else
				update_flag = 0;
			tmpobj = cJSON_GetObjectItem(versionobj, "updateChecks");
			if (tmpobj)
			{
				if (tmpobj->type == cJSON_True)
					doUpdates = true;
				else
					doUpdates = false;
			}
		}
		else
		{
			last_major = 0;
			last_minor = 0;
			last_build = 0;
			update_flag = 0;
		}
		
		//Read FavMenu
		tmpobj = cJSON_GetObjectItem(root, "Favorites");
		if (tmpobj)
		{
			int favSize = cJSON_GetArraySize(tmpobj);
			for (int i = 0; i < favSize; i++)
			{
				std::string identifier = cJSON_GetArrayItem(tmpobj, i)->valuestring;
				Favorite::Ref().AddFavorite(identifier);
			}
		}
		
		//Read Records
		recobj = cJSON_GetObjectItem(root, "records");
		if (recobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(recobj, "Total Time Played")))
				totaltime = (int)((tmpobj->valuedouble)*1000);
			if ((tmpobj = cJSON_GetObjectItem(recobj, "Average FPS")))
				totalfps = tmpobj->valuedouble;
			if ((tmpobj = cJSON_GetObjectItem(recobj, "Number of frames")))
			{
				frames = tmpobj->valueint;
				totalfps = totalfps * frames;
			}
			if ((tmpobj = cJSON_GetObjectItem(recobj, "Total AFK Time")))
				prevafktime = (int)((tmpobj->valuedouble)*1000);
			if ((tmpobj = cJSON_GetObjectItem(recobj, "Times Played")))
				timesplayed = tmpobj->valueint;
		}

		//Read display settings
		graphicsobj = cJSON_GetObjectItem(root, "Renderer");
		if (graphicsobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "ColourMode")))
				Renderer::Ref().SetColorMode(tmpobj->valueint);
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "DisplayMode")))
				Renderer::Ref().SetDisplayMode(tmpobj->valueint);
			else if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "DisplayModes")))
			{
				int count = cJSON_GetArraySize(tmpobj);
				cJSON * tempDisplayMode;
				unsigned int arrTotal = 0;
				for (int i = 0; i < count; i++)
				{
					tempDisplayMode = cJSON_GetArrayItem(tmpobj, i);
					unsigned int mode = tempDisplayMode->valueint;
					if (mode == 0)
					{
						char * strMode = tempDisplayMode->valuestring;
						if (strMode && strlen(strMode))
							mode = atoi(strMode);
					}
					arrTotal |= mode;
				}
				Renderer::Ref().SetDisplayMode(arrTotal);
			}
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "RenderMode")))
			{
				unsigned int mode = (unsigned int) tmpobj->valuedouble;
				Renderer::Ref().SetRenderMode(mode);
			}
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "RenderModes")))
			{
				int count = cJSON_GetArraySize(tmpobj);
				cJSON * tempRenderMode;
				unsigned int arrTotal = 0;
				for (int i = 0; i < count; i++)
				{
					tempRenderMode = cJSON_GetArrayItem(tmpobj, i);
					unsigned int mode = tempRenderMode->valueint;
					if (mode == 0)
					{
						char * strMode = tempRenderMode->valuestring;
						if (strMode && strlen(strMode))
							mode = atoi(strMode);
					}
					// temporary hack until I update the json library (first for loading current modes, second only needed for loading with valuestring)
					if (mode == 2147483648u || mode == 2147483647u)
						mode = 4278252144u;
					arrTotal |= mode;
				}
				Renderer::Ref().SetRenderMode(arrTotal);
			}
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "Decorations")) && tmpobj->type == cJSON_True)
				decorations_enable = true;
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "GravityField")) && tmpobj->type == cJSON_True)
				drawgrav_enable = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "DebugMode")) && tmpobj->type == cJSON_True)
				DEBUG_MODE = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(graphicsobj, "TemperatureScale")))
				globalSim->temperatureScale = tmpobj->valueint;
		}

		//Read simulation settings
		simulationobj = cJSON_GetObjectItem(root, "Simulation");
		if (simulationobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "EdgeMode")))
			{
				char edgeMode = (char)tmpobj->valueint;
				if (edgeMode > 3)
					edgeMode = 0;
				globalSim->edgeMode = edgeMode;
			}
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "AmbientAirTemp")))
				globalSim->air->SetAmbientAirTempPref(tmpobj->valuedouble);
#ifndef ANDROID
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "NewtonianGravity")) && tmpobj->valueint == 1)
				globalSim->grav->StartAsync();
#endif
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "AmbientHeat")))
				aheat_enable = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "PrettyPowder")))
				pretty_powder = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "UndoHistoryLimit")))
				SnapshotHistory::SetUndoHistoryLimit(tmpobj->valueint);
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "LoadPressure")))
				globalSim->includePressure = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "DecoSpace")))
				globalSim->decoSpace = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(simulationobj, "RealisticHeat")))
				realistic = tmpobj->valueint;
		}

		//read console history
		consoleobj = cJSON_GetObjectItem(root, "Console");
		if (consoleobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(consoleobj, "History")))
			{
				cJSON *resultsObj = cJSON_GetObjectItem(consoleobj, "HistoryResults");
				load_console_history(tmpobj, resultsObj);
			}
		}

		itemobj = cJSON_GetObjectItem(root, "NativeClipboard");
		if (itemobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "Enabled")))
			{
				nativeClipboard = tmpobj->valueint ? true : false;
			}
		}

		//Read HUDs
		hudobj = cJSON_GetObjectItem(root, "HUD");
		if (hudobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(hudobj, "normal")))
			{
				int count = std::min(HUD_OPTIONS,cJSON_GetArraySize(tmpobj));
				for (int i = 0; i < count; i++)
				{
					normalHud[i] = cJSON_GetArrayItem(tmpobj, i)->valueint;
				}
			}
			if ((tmpobj = cJSON_GetObjectItem(hudobj, "debug")))
			{
				int count = std::min(HUD_OPTIONS,cJSON_GetArraySize(tmpobj));
				for (int i = 0; i < count; i++)
				{
					debugHud[i] = cJSON_GetArrayItem(tmpobj, i)->valueint;
				}
			}
		}
		
		//Read general settings
		if ((tmpobj = cJSON_GetObjectItem(root, "Proxy")) && tmpobj->type == cJSON_String)
			strncpy(http_proxy_string, tmpobj->valuestring, 255);
		else
			http_proxy_string[0] = 0;
		if ((tmpobj = cJSON_GetObjectItem(root, "Scale")))
		{
			int scale = tmpobj->valueint;
			if (scale == 0)
				scale = cJSON_GetInt(&tmpobj);
			if (scale >= 0 && scale <= 5)
				Engine::Ref().SetScale(scale);
		}
		if ((tmpobj = cJSON_GetObjectItem(root, "Resizable")))
			Engine::Ref().SetResizable(tmpobj->valueint ? true : false, false);
		if ((tmpobj = cJSON_GetObjectItem(root, "ResizableMode")))
			Engine::Ref().SetPixelFilteringMode(tmpobj->valueint, false);
		if ((tmpobj = cJSON_GetObjectItem(root, "Fullscreen")))
			Engine::Ref().SetFullscreen(tmpobj->valueint ? true : false);
		if ((tmpobj = cJSON_GetObjectItem(root, "AltFullscreen")))
			Engine::Ref().SetAltFullscreen(tmpobj->valueint ? true : false);
		if ((tmpobj = cJSON_GetObjectItem(root, "ForceIntegerScaling")))
			Engine::Ref().SetForceIntegerScaling(tmpobj->valueint ? true : false);
		if ((tmpobj = cJSON_GetObjectItem(root, "MomentumScroll")))
			Engine::Ref().SetMomentumScroll(tmpobj->valueint ? true : false);
		if ((tmpobj = cJSON_GetObjectItem(root, "FastQuit")))
			Engine::Ref().SetFastQuit(tmpobj->valueint ? true : false);
		if ((tmpobj = cJSON_GetObjectItem(root, "WindowX")))
			savedWindowX = tmpobj->valueint;
		if ((tmpobj = cJSON_GetObjectItem(root, "WindowY")))
			savedWindowY = tmpobj->valueint;
		if ((tmpobj = cJSON_GetObjectItem(root, "MouseClickRequired")))
			stickyCategories = tmpobj->valueint ? true : false;
		if ((tmpobj = cJSON_GetObjectItem(root, "PerfectCircleBrush")))
			perfectCircleBrush = tmpobj->valueint ? true : false;
		if ((tmpobj = cJSON_GetObjectItem(root, "GraveExitsConsole")))
			graveExitsConsole = tmpobj->valueint ? true : false;

		//Read some extra mod settings
		if ((tmpobj = cJSON_GetObjectItem(root, "heatmode")))
			heatmode = tmpobj->valueint;
		if (heatmode == 2)
		{
			if ((tmpobj = cJSON_GetObjectItem(root, "HeatModeLowerBound")))
				lowesttemp = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(root, "HeatModeUpperBound")))
				highesttemp = tmpobj->valueint;
		}
		if ((tmpobj = cJSON_GetObjectItem(root, "autosave")))
			autosave = tmpobj->valueint;
#ifndef NOMOD
		if ((tmpobj = cJSON_GetObjectItem(root, "EXPL_unlocked")))
			explUnlocked = true;
#endif
/*#ifndef TOUCHUI
		if ((tmpobj = cJSON_GetObjectItem(root, "old_menu")))
			old_menu = 1;
#endif*/
		if ((tmpobj = cJSON_GetObjectItem(root, "alt_find")))
			finding |= 0x8;
		if ((tmpobj = cJSON_GetObjectItem(root, "dateformat")))
			dateformat = tmpobj->valueint;
		if ((tmpobj = cJSON_GetObjectItem(root, "decobox_hidden")))
			decobox_hidden = tmpobj->valueint;
		if ((tmpobj = cJSON_GetObjectItem(root, "loadIncompatibleSaves")))
			loadIncompatibleSaves = true;
		if ((tmpobj = cJSON_GetObjectItem(root, "LuaHookTimeout")))
			luaHookTimeout = tmpobj->valueint;

		itemobj = cJSON_GetObjectItem(root, "SavePreview");
		if (itemobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "scrollSpeed")) && tmpobj->valueint > 0)
				scrollSpeed = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "scrollSpeedMomentum")) && tmpobj->valueint > 0)
				scrollSpeedMomentum = tmpobj->valueint;
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "scrollDeceleration")) && tmpobj->valuedouble >= 0 && tmpobj->valuedouble <= 1)
				scrollDeceleration = tmpobj->valuedouble;
			if ((tmpobj = cJSON_GetObjectItem(itemobj, "ShowIDs")))
				show_ids = tmpobj->valueint;
		}

		customgolobj = cJSON_GetObjectItem(root, "CustomGOL");
		if (customgolobj)
		{
			if ((tmpobj = cJSON_GetObjectItem(customgolobj, "Name")) && tmpobj->type == cJSON_String)
				static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).SetCachedName(tmpobj->valuestring);
			if ((tmpobj = cJSON_GetObjectItem(customgolobj, "Rule")) && tmpobj->type == cJSON_String)
				static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).SetCachedRuleString(tmpobj->valuestring);

			if ((tmpobj = cJSON_GetObjectItem(customgolobj, "Types")) && tmpobj->type == cJSON_Array)
			{
				int count = cJSON_GetArraySize(tmpobj);
				cJSON * tempCustomGol;
				for (int i = 0; i < count; i++)
				{
					tempCustomGol = cJSON_GetArrayItem(tmpobj, i);
					if (tempCustomGol->type != cJSON_String)
						continue;

					CustomGOLData cgol;
					std::string customGol = tempCustomGol->valuestring;
					std::string color1Str, color2Str;

					int pos, pos2;
					pos2 = customGol.find(' ');
					if (pos2 == -1) continue;
					cgol.nameString = customGol.substr(0, pos2);
					pos = pos2 + 1;

					pos2 = customGol.find(' ', pos);
					if (pos2 == -1) continue;
					cgol.ruleString = customGol.substr(pos, pos2 - pos);
					pos = pos2 + 1;

					pos2 = customGol.find(' ', pos);
					if (pos2 == -1) continue;
					color1Str = customGol.substr(pos, pos2 - pos);
					pos = pos2 + 1;

					pos2 = customGol.find(' ', pos);
					if (pos2 != -1) continue;
					color2Str = customGol.substr(pos);

					if (!ValidateGOLName(cgol.nameString))
						continue;

					cgol.rule = ParseGOLString(cgol.ruleString);
					if (cgol.rule == -1)
						continue;

					cgol.color1 = Format::StringToNumber<int>(color1Str);
					cgol.color2 = Format::StringToNumber<int>(color2Str);

					static_cast<LIFE_ElementDataContainer&>(*globalSim->elementData[PT_LIFE]).AddCustomGOL(cgol);
				}
			}
		}

		cJSON_Delete(root);
		free(prefdata);

		// settings that need to be changed on specific updates can go here
		//if (update_flag)
		{
#ifdef WIN
			if (last_build == 322)
			{
				scrollSpeed = 15;
				scrollDeceleration = 0.99f;
			}
#endif
			// New defaults which match vanilla tpt
			if (last_build <= 345)
			{
				scrollSpeed = 15;
				scrollDeceleration = 0.98f;
			}
			// With the addition of temperature scale, turn off Celsius/Fahrenheit/Kelvin displays in hud and turn on default scale display
			if (last_modbuild <= 149)
			{
				normalHud[14] = 1;
				normalHud[15] = 0;
				normalHud[16] = 0;
				normalHud[17] = 0;
				debugHud[14] = 1;
				debugHud[15] = 0;
				debugHud[16] = 0;
				debugHud[17] = 0;
			}
		}
	}
	else
		firstRun = true;
}

int sregexp(const char *str, const char *pattern)
{
	int result;
	regex_t patternc;
	if (regcomp(&patternc, pattern,  0)!=0)
		return 1;
	result = regexec(&patternc, str, 0, NULL, 0);
	regfree(&patternc);
	return result;
}

void strcaturl(char *dst, char *src)
{
	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		if ((*s>='0' && *s<='9') ||
		        (*s>='a' && *s<='z') ||
		        (*s>='A' && *s<='Z'))
			*(d++) = *s;
		else
		{
			*(d++) = '%';
			*(d++) = hex[*s>>4];
			*(d++) = hex[*s&15];
		}
	}
	*d = 0;
}

void strappend(char *dst, const char *src)
{
	char *d;
	unsigned char *s;

	for (d=dst; *d; d++) ;

	for (s=(unsigned char *)src; *s; s++)
	{
		*(d++) = *s;
	}
	*d = 0;
}

void *file_load(const char *fn, int *size)
{
	FILE *f = fopen(fn, "rb");
	void *s;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	s = malloc(*size);
	if (!s)
	{
		fclose(f);
		return NULL;
	}
	int readsize = fread(s, *size, 1, f);
	fclose(f);
	if (readsize != 1)
	{
		free(s);
		return NULL;
	}
	return s;
}

void HSV_to_RGB(int h,int s,int v,int *r,int *g,int *b)//convert 0-255(0-360 for H) HSV values to 0-255 RGB
{
	float hh, ss, vv, c, x;
	int m;
	hh = h/60.0f;//normalize values
	ss = s/255.0f;
	vv = v/255.0f;
	c = vv * ss;
	x = c * ( 1 - fabs(fmod(hh,2.0f) -1) );
	if(hh<1){
		*r = (int)(c*255.0);
		*g = (int)(x*255.0);
		*b = 0;
	}
	else if(hh<2){
		*r = (int)(x*255.0);
		*g = (int)(c*255.0);
		*b = 0;
	}
	else if(hh<3){
		*r = 0;
		*g = (int)(c*255.0);
		*b = (int)(x*255.0);
	}
	else if(hh<4){
		*r = 0;
		*g = (int)(x*255.0);
		*b = (int)(c*255.0);
	}
	else if(hh<5){
		*r = (int)(x*255.0);
		*g = 0;
		*b = (int)(c*255.0);
	}
	else if(hh<6){
		*r = (int)(c*255.0);
		*g = 0;
		*b = (int)(x*255.0);
	}
	m = (int)((vv-c)*255.0);
	*r += m;
	*g += m;
	*b += m;
}

void RGB_to_HSV(int r,int g,int b,int *h,int *s,int *v)//convert 0-255 RGB values to 0-255(0-360 for H) HSV
{
	float rr, gg, bb, a,x,c,d;
	rr = r/255.0f;//normalize values
	gg = g/255.0f;
	bb = b/255.0f;
	a = std::min(rr,gg);
	a = std::min(a,bb);
	x = std::max(rr,gg);
	x = std::max(x,bb);
	if (r == g && g == b)//greyscale
	{
		*h = 0;
		*s = 0;
		*v = (int)(a*255.0);
	}
	else
	{
		int min = (r < g) ? ((r < b) ? 0 : 2) : ((g < b) ? 1 : 2);
		c = (min==0) ? gg-bb : ((min==2) ? rr-gg : bb-rr);
		d = (float)((min==0) ? 3 : ((min==2) ? 1 : 5));
		*h = (int)(60.0*(d - c/(x - a)));
		*s = (int)(255.0*((x - a)/x));
		*v = (int)(255.0*x);
	}
}

Tool* GetToolFromIdentifier(const std::string &identifier)
{
	for (int i = 0; i < SC_TOTAL; i++)
	{
		for (auto *tool : menuSections[i]->tools)
		{
			if  (identifier == tool->GetIdentifier())
				return tool;
		}
	}
	return nullptr;
}

void membwand(void * destv, void * srcv, size_t destsize, size_t srcsize)
{
	size_t i;
	unsigned char * dest = (unsigned char*)destv;
	unsigned char * src = (unsigned char*)srcv;
	if (srcsize==destsize)
	{
		for(i = 0; i < destsize; i++){
			dest[i] = dest[i] & src[i];
		}
	}
	else
	{
		for(i = 0; i < destsize; i++){
			dest[i] = dest[i] & src[i%srcsize];
		}
	}
}

bool byteStringEqualsString(const std::string &str, const char *data, size_t size)
{
	return str.size() == size && !memcmp(str.data(), data, size);
}
