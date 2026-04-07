#ifndef TOOLNUMBERS_H
#define TOOLNUMBERS_H
#include <string>
#include "graphics/ARGBColour.h"

#define TOOL_HEAT	0
#define TOOL_COOL	1
#define TOOL_AIR	2
#define TOOL_VAC	3
#define TOOL_PGRV	4
#define TOOL_NGRV	5
#define TOOL_MIX	6
#define TOOL_CYCL	7
#define TOOL_AMBM	8
#define TOOL_AMBP	9
#define TOOL_WIND	10
#define TOOL_PROP	11
#define TOOL_SIGN	12
#define TOOL_GOL	13
#define TOOLCOUNT	14

#define OLD_PT_WIND	147
#define OLD_WL_SIGN	126
#define OLD_SPC_AIR	236
#define SPC_AIR		256

struct toolType
{
	std::string name;
	std::string identifier;
	ARGBColour color;
	std::string descs;
};
typedef struct toolType toolType;

static toolType toolTypes[] =
{
	{"HEAT", "DEFAULT_TOOL_HEAT",   COLPACK(0xFFBB00), "Heats the targeted element."},
	{"COOL", "DEFAULT_TOOL_COOL",   COLPACK(0x00BBFF), "Cools the targeted element."},
	{"AIR",  "DEFAULT_TOOL_AIR",    COLPACK(0xFFFFFF), "Air, creates airflow and pressure."},
	{"VAC",  "DEFAULT_TOOL_VAC",    COLPACK(0x303030), "Vacuum, reduces air pressure."},
	{"PGRV", "DEFAULT_TOOL_PGRV",   COLPACK(0xCCCCFF), "Creates a short-lasting gravity well."},
	{"NGRV", "DEFAULT_TOOL_NGRV",   COLPACK(0xAACCFF), "Creates a short-lasting negative gravity well."},
	{"MIX",  "DEFAULT_TOOL_MIX",    COLPACK(0xFFD090), "Mixes particles around randomly."},
	{"CYCL", "DEFAULT_TOOL_CYCL",   COLPACK(0x132F5B), "Cyclone, produces swirling air currents."},
	{"AMBM", "DEFAULT_TOOL_AMBM",   COLPACK(0x00DDFF), "Decreases ambient air temperature."},
	{"AMBP", "DEFAULT_TOOL_AMBP",   COLPACK(0xFFDD00), "Increases ambient air temperature."},
	{"WIND", "DEFAULT_TOOL_WIND",   COLPACK(0x404040), "Creates air movement."},
	{"PROP", "DEFAULT_UI_PROPERTY", COLPACK(0xFFAA00), "Property drawing tool."},
	{"SIGN", "DEFAULT_UI_SIGN",     COLPACK(0x808080), "Sign. Displays text. Click on a sign to edit it or anywhere else to place a new one."},
	{"CUST", "DEFAULT_UI_ADDLIFE",  COLPACK(0xFEA900), "Add a new custom GOL type. (Use ctrl+shift+rightclick to remove them)"}
};

#define DECO_DRAW		0
#define DECO_CLEAR		1
#define DECO_ADD		2
#define DECO_SUBTRACT	3
#define DECO_MULTIPLY	4
#define DECO_DIVIDE		5
#define DECO_SMUDGE		6
#define DECO_LIGHTEN	7
#define DECO_DARKEN		8
#define DECOCOUNT		9

struct decoType
{
	std::string name;
	std::string identifier;
	ARGBColour color;
	std::string descs;
};
typedef struct decoType decoType;

static decoType decoTypes[] =
{
	{"SET", "DEFAULT_DECOR_SET",	COLPACK(0xFF0000), "Draw decoration."},
	{"CLR", "DEFAULT_DECOR_CLR",	COLPACK(0x000000), "Erase decoration."},
	{"ADD", "DEFAULT_DECOR_ADD",	COLPACK(0x323232), "Color blending: Add."},
	{"SUB", "DEFAULT_DECOR_SUB",	COLPACK(0x323232), "Color blending: Subtract."},
	{"MUL", "DEFAULT_DECOR_MUL",	COLPACK(0x323232), "Color blending: Multiply."},
	{"DIV", "DEFAULT_DECOR_DIV",	COLPACK(0x323232), "Color blending: Divide."},
	{"SMDG", "DEFAULT_DECOR_SMDG",	COLPACK(0x00FF00), "Smudge tool, blends surrounding deco together."},
	{"LIGH", "DEFAULT_DECOR_LIGH",	COLPACK(0xDDDDDD), "Lighten deco color."},
	{"DARK", "DEFAULT_DECOR_DARK",	COLPACK(0x111111), "Darken deco color."}
};

struct decoPreset
{
	ARGBColour colour;
	std::string identifier;
	std::string descs;
};
typedef struct decoPreset decoPreset;

const decoPreset colorlist[] =
{
	{COLPACK(0xFF0000), "DEFAULT_DECOR_PRESET_RED", "Red"},
	{COLPACK(0x00FF00), "DEFAULT_DECOR_PRESET_GREEN", "Green"},
	{COLPACK(0x0000FF), "DEFAULT_DECOR_PRESET_BLUE", "Blue"},
	{COLPACK(0xFFFF00), "DEFAULT_DECOR_PRESET_YELLOW", "Yellow"},
	{COLPACK(0xFF00FF), "DEFAULT_DECOR_PRESET_PINK", "Pink"},
	{COLPACK(0x00FFFF), "DEFAULT_DECOR_PRESET_CYAN", "Cyan"},
	{COLPACK(0xFFFFFF), "DEFAULT_DECOR_PRESET_WHITE", "White"},
	{COLPACK(0x000000), "DEFAULT_DECOR_PRESET_BLACK", "Black"},
};
#define NUM_COLOR_PRESETS 8


//fav menu stuff since doesn't go anywhere else
#define FAV_MORE 0
#define FAV_BACK 1
#define FAV_FIND 2
#define FAV_INFO 3
#define FAV_ROTATE 4
#define FAV_HEAT 5
#define FAV_LUA 6
#define FAV_CUSTOMHUD 7
#define FAV_REAL 8
#define FAV_FIND2 9
#define FAV_DATE 10
#define FAV_SECR 11
#define FAV_END 12
#define NUM_FAV_BUTTONS 12

struct fav_menu
{
	const char *name;
	ARGBColour colour;
	std::string description;
	std::string identifier;
};
typedef struct fav_menu fav_menu;

const fav_menu fav[] =
{
	{"MORE", COLPACK(0xFF7F00), "Display different options", "DEFAULT_FAV_MORE"},
	{"BACK", COLPACK(0xFF7F00), "Go back to the favorites menu", "DEFAULT_FAV_BACK"},
	{"FIND", COLPACK(0xFF0000), "Highlights the currently selected element in red (ctrl+f)", "DEFAULT_FAV_FIND"},
	{"INFO", COLPACK(0x00FF00), "Displays statistics and records about The Powder Toy. Left click to toggle display", "DEFAULT_FAV_INFO"},
	{"SPIN", COLPACK(0x0010A0), "Makes moving solids rotate. Currently ", "DEFAULT_FAV_SPIN"},
	{"HEAT", COLPACK(0xFF00D4), "Changes heat display mode. Right click to set manual temperatures. Current mode: ", "DEFAULT_FAV_HEAT"},
	{"LUA",  COLPACK(0xFFFF00), "Add Lua code to a save", "DEFAULT_FAV_LUA"},
	{"HUD2", COLPACK(0x20D8FF), "Make a custom HUD", "DEFAULT_FAV_HUD2"},
	{"REAL", COLPACK(0xFF6800), "Turns on realistic heat mode, by savask. Now ", "DEFAULT_FAV_REAL"},
	{"FND2", COLPACK(0xDF0000), "Alternate find mode, looks different but may find things better. Now ", "DEFAULT_FAV_FND2"},
	{"DATE", COLPACK(0x3FBB3F), "Change date and time format. Right click to toggle always showing time. Example: ", "DEFAULT_FAV_DATE"},
	{"", COLPACK(0x000000), "", "DEFAULT_FAV_SECRET"}
};

#endif
