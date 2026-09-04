#ifndef WALLNUMBERS_H
#define WALLNUMBERS_H
#include <string>
#include <graphics/ARGBColour.h>

#define O_WL_WALLELEC     122
#define O_WL_EWALL        123
#define O_WL_DETECT       124
#define O_WL_STREAM       125
#define O_WL_SIGN         126
#define O_WL_FAN          127
#define O_WL_FANHELPER    255
#define O_WL_ALLOWLIQUID  128
#define O_WL_DESTROYALL   129
#define O_WL_ERASE        130
#define O_WL_WALL         131
#define O_WL_ALLOWAIR     132
#define O_WL_ALLOWSOLID   133
#define O_WL_ALLOWALLELEC 134
#define O_WL_EHOLE        135
#define O_WL_ALLOWGAS     140
#define O_WL_GRAV         142
#define O_WL_ALLOWENERGY  145
#define O_WL_ERASEALL     147

#define WL_ERASE        0
#define WL_WALLELEC     1
#define WL_EWALL        2
#define WL_DETECT       3
#define WL_STREAM       4
#define WL_FAN          5
#define WL_ALLOWLIQUID  6
#define WL_DESTROYALL   7
#define WL_WALL         8
#define WL_ALLOWAIR     9
#define WL_ALLOWPOWDER  10
#define WL_ALLOWALLELEC 11
#define WL_EHOLE        12
#define WL_ALLOWGAS     13
#define WL_GRAV         14
#define WL_ALLOWENERGY  15
#define WL_BLOCKAIR     16
#define WL_ERASEALL     17
#define WL_STASIS       18
#define WL_FANHELPER    255

#define WALLCOUNT 19

struct wallType
{
	std::string name;
	std::string identifier;
	ARGBColour colour;
	ARGBColour eglow; // if emap set, add this to fire glow
	int drawstyle;
	std::string descs;
};
typedef struct wallType wallType;

const wallType wallTypes[] =
{
	{"ERASE",           "DEFAULT_WL_ERASE", COLPACK(0x808080), COLPACK(0x000000), -1, "擦除墙体。"},
	{"CONDUCTIVE WALL", "DEFAULT_WL_CNDTW", COLPACK(0xC0C0C0), COLPACK(0x101010), 0,  "阻挡所有物质，并可导电。"},
	{"EWALL",           "DEFAULT_WL_EWALL", COLPACK(0x808080), COLPACK(0x808080), 0,  "电子墙。通电时允许粒子通过。"},
	{"DETECTOR",        "DEFAULT_WL_DTECT", COLPACK(0xFF8080), COLPACK(0xFF2008), 1,  "探测墙。内部出现粒子时产生电流。"},
	{"STREAMLINE",      "DEFAULT_WL_STRM",  COLPACK(0x808080), COLPACK(0x000000), 0,  "流线。绘制一条跟随空气运动的轨迹。"},
#ifndef TOUCHUI
	{"FAN",             "DEFAULT_WL_FAN",   COLPACK(0x8080FF), COLPACK(0x000000), 1,  "风扇。推动空气；用直线工具设置方向和强度。"},
#else
	{"FAN",             "DEFAULT_WL_FAN",   COLPACK(0x8080FF), COLPACK(0x000000), 1,  "风扇。推动空气；Android 版无法设置风力。"},
#endif
	{"LIQUID WALL",     "DEFAULT_WL_LIQD",  COLPACK(0xC0C0C0), COLPACK(0x101010), 2,  "仅允许液体通过，并可导电。"},
	{"ABSORB WALL",     "DEFAULT_WL_ABSRB", COLPACK(0x808080), COLPACK(0x000000), 1,  "吸收粒子，但允许气流通过。"},
	{"WALL",            "DEFAULT_WL_WALL",  COLPACK(0x808080), COLPACK(0x000000), 3,  "基础墙体，阻挡所有物质。"},
	{"AIRONLY WALL",    "DEFAULT_WL_AIR",   COLPACK(0x3C3C3C), COLPACK(0x000000), 1,  "允许空气通过，但阻挡所有粒子。"},
	{"POWDER WALL",     "DEFAULT_WL_POWDR", COLPACK(0x575757), COLPACK(0x000000), 1,  "仅允许粉末通过。"},
	{"CONDUCTOR",       "DEFAULT_WL_CNDTR", COLPACK(0xFFFF22), COLPACK(0x101010), 2,  "导电墙。允许所有粒子通过，并可导电。"},
	{"EHOLE",           "DEFAULT_WL_EHOLE", COLPACK(0x242424), COLPACK(0x101010), 0,  "电子洞。吸收粒子，通电时将其释放。"},
	{"GAS WALL",        "DEFAULT_WL_GAS",   COLPACK(0x579777), COLPACK(0x000000), 1,  "仅允许气体通过。"},
	{"GRAVITY WALL",    "DEFAULT_WL_GRVTY", COLPACK(0xFFEE00), COLPACK(0xAA9900), 4,  "重力墙。由它围成的区域不受牛顿引力影响。"},
	{"ENERGY WALL",     "DEFAULT_WL_ENRGY", COLPACK(0xFFAA00), COLPACK(0xAA5500), 4,  "仅允许能量粒子通过。"},
	{"AIRBLOCK WALL",   "DEFAULT_WL_NOAIR", COLPACK(0xDCDCDC), COLPACK(0x000000), 1,  "允许所有粒子通过，但阻挡空气。"},
	{"ERASEALL",        "DEFAULT_WL_ERASEA",COLPACK(0x808080), COLPACK(0x000000), -1, "擦除墙体、粒子和标牌。"},
	{"STASIS WALL",     "DEFAULT_WL_STASIS",COLPACK(0x800080), COLPACK(0x000000), 0,  "冻结墙内粒子；通电后解除冻结。"},
};

#endif
