#include "LuaTool.h"
#include "luascriptinterface.h"
#include "common/Point.h"
#include "game/Brush.h"
#include "simulation/Simulation.h"
#include "gui/game/PowderToy.h"

std::map<int, LuaToolData> luaTools;
std::map<int, CustomTool> luaToolRefs;

LuaTool::LuaTool(int index, std::string name, ARGBColour color, std::string identifier, std::string description, int menuSection):
	Tool(CUSTOM_TOOL, -1, identifier, name, description, color, menuSection),
	index(index)
{

}

void LuaTool::CallPerform(Simulation *sim, Point position, Point brushOffset, float strength)
{
	particle * cpart = nullptr;
	int r;
	if ((r = pmap[position.Y][position.X]))
		cpart = &(sim->parts[ID(r)]);
	else if ((r = photons[position.Y][position.X]))
		cpart = &(sim->parts[ID(r)]);
	// TODO: maybe do something with the result
	luaPerformWrapper(this, sim, cpart, position.X, position.Y, brushOffset.X, brushOffset.Y, strength);
}

int LuaTool::luaPerformWrapper(LuaTool *tool, Simulation *sim, particle *cpart, int x, int y, int brushX, int brushY, float strength)
{
	int ok = 0;
	int index = tool->GetIndex();
	if (!index)
		return 0;
	if (luaToolRefs[index].perform)
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].perform);
		if (cpart)
		{
			lua_pushinteger(l, cpart - &luaSim->parts[0]);
		}
		else
		{
			lua_pushnil(l);
		}
		lua_pushinteger(l, x);
		lua_pushinteger(l, y);
		lua_pushnumber(l, the_game->GetToolStrength());
		lua_pushboolean(l, the_game->IsShiftHeld());
		lua_pushboolean(l, the_game->IsCtrlHeld());
		lua_pushboolean(l, the_game->IsAltHeld());
		lua_pushinteger(l, brushX);
		lua_pushinteger(l, brushY);
		if (tpt_lua_pcall(l, 9, 1, 0, eventTraitNone))
		{
			luacon_log("In perform func: " + luacon_geterror());
			lua_pop(l, 1);
		}
		else
		{
			if (lua_isboolean(l, -1))
			{
				ok = lua_toboolean(l, -1);
			}
			lua_pop(l, 1);
		}
	}
	return ok;
}

void LuaTool::luaClickWrapper(int index, Simulation *sim, Brush *brush, Point position)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].click);
	lua_pushinteger(l, brush->GetShape());
	lua_pushinteger(l, position.X);
	lua_pushinteger(l, position.Y);
	lua_pushnumber(l, the_game->GetToolStrength());
	lua_pushboolean(l, the_game->IsShiftHeld());
	lua_pushboolean(l, the_game->IsCtrlHeld());
	lua_pushboolean(l, the_game->IsAltHeld());
	if (tpt_lua_pcall(l, 7, 0, 0, eventTraitNone))
	{
		luacon_log("In click func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void LuaTool::luaDragWrapper(int index, Simulation *sim, Brush *brush, Point position1, Point position2)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].drag);
	lua_pushinteger(l, brush->GetShape());
	lua_pushinteger(l, position1.X);
	lua_pushinteger(l, position1.Y);
	lua_pushinteger(l, position2.X);
	lua_pushinteger(l, position2.Y);
	lua_pushnumber(l, the_game->GetToolStrength());
	lua_pushboolean(l, the_game->IsShiftHeld());
	lua_pushboolean(l, the_game->IsCtrlHeld());
	lua_pushboolean(l, the_game->IsAltHeld());
	if (tpt_lua_pcall(l, 9, 0, 0, eventTraitNone))
	{
		luacon_log("In drag func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void LuaTool::luaDrawWrapper(int index, Simulation *sim, Brush *brush, Point position)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].draw);
	lua_pushinteger(l, brush->GetShape());
	lua_pushinteger(l, position.X);
	lua_pushinteger(l, position.Y);
	lua_pushnumber(l, the_game->GetToolStrength());
	lua_pushboolean(l, the_game->IsShiftHeld());
	lua_pushboolean(l, the_game->IsCtrlHeld());
	lua_pushboolean(l, the_game->IsAltHeld());
	if (tpt_lua_pcall(l, 7, 0, 0, eventTraitNone))
	{
		luacon_log("In draw func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void LuaTool::luaDrawLineWrapper(int index, Simulation *sim, Brush *brush, Point position1, Point position2, bool dragging)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].drawLine);
	lua_pushinteger(l, brush->GetShape());
	lua_pushinteger(l, position1.X);
	lua_pushinteger(l, position1.Y);
	lua_pushinteger(l, position2.X);
	lua_pushinteger(l, position2.Y);
	lua_pushnumber(l, the_game->GetToolStrength());
	lua_pushboolean(l, the_game->IsShiftHeld());
	lua_pushboolean(l, the_game->IsCtrlHeld());
	lua_pushboolean(l, the_game->IsAltHeld());
	if (tpt_lua_pcall(l, 9, 0, 0, eventTraitNone))
	{
		luacon_log("In drawLine func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void LuaTool::luaDrawRectWrapper(int index, Simulation *sim, Brush *brush, Point position1, Point position2)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].drawRect);
	lua_pushinteger(l, brush->GetShape());
	lua_pushinteger(l, position1.X);
	lua_pushinteger(l, position1.Y);
	lua_pushinteger(l, position2.X);
	lua_pushinteger(l, position2.Y);
	lua_pushnumber(l, the_game->GetToolStrength());
	lua_pushboolean(l, the_game->IsShiftHeld());
	lua_pushboolean(l, the_game->IsCtrlHeld());
	lua_pushboolean(l, the_game->IsAltHeld());
	if (tpt_lua_pcall(l, 9, 0, 0, eventTraitNone))
	{
		luacon_log("In drawRect func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void LuaTool::luaDrawFillWrapper(int index, Simulation *sim, Brush *brush, Point position)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].drawFill);
	lua_pushinteger(l, brush->GetShape());
	lua_pushinteger(l, position.X);
	lua_pushinteger(l, position.Y);
	lua_pushnumber(l, 1.0f /*the_game->GetToolStrength()*/);
	lua_pushboolean(l, false /*the_game->IsShiftHeld()*/);
	lua_pushboolean(l, false /*the_game->IsCtrlHeld()*/);
	lua_pushboolean(l, false /*the_game->IsAltHeld()*/);
	if (tpt_lua_pcall(l, 7, 0, 0, eventTraitNone))
	{
		luacon_log("In drawFill func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void LuaTool::luaSelectWrapper(int index, int toolSelection)
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, luaToolRefs[index].select);
	lua_pushinteger(l, toolSelection);
	if (tpt_lua_pcall(l, 1, 0, 0, eventTraitNone))
	{
		luacon_log("In select func: " + luacon_geterror());
		lua_pop(l, 1);
	}
}

void defaultPerformDraw(LuaTool *tool, Simulation *sim, Brush *brush, Point position, float strength)
{
	int x = position.X, y = position.Y;
	int rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
	if (rx <= 0) //workaround for rx == 0 crashing. todo: find a better fix later.
	{
		for (int j = y - ry; j <= y + ry; j++)
			tool->CallPerform(sim, { x, j }, position, strength);
	}
	else
	{
		int tempy = y - 1, i, j, jmax;
		// tempy is the closest y value that is just outside (above) the brush
		// jmax is the closest y value that is just outside (below) the brush

		// For triangle brush, start at the very bottom
		if (brush->GetShape() == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++)
		{
			//loop up until it finds a point not in the brush
			while (tempy >= y - ry && brush->IsInside(i - x, tempy - y))
				tempy = tempy - 1;

			// If triangle brush, create parts down to the bottom always; if not go down to the bottom border
			if (brush->GetShape() == TRI_BRUSH)
				jmax = y + ry;
			else
				jmax = 2 * y - tempy;
			for (j = tempy + 1; j < jmax; j++)
			{
				tool->CallPerform(sim, { i, j }, position, strength);
				// Don't create twice in the vertical center line
				if (i != x)
					tool->CallPerform(sim, { 2*x-i, j }, position, strength);
			}
		}
	}
}

void defaultPerformDrawLine(LuaTool *tool, Simulation *sim, Brush *brush, Point position1, Point position2, bool dragging, float strength)
{
	auto x1 = position1.X;
	auto y1 = position1.Y;
	auto x2 = position2.X;
	auto y2 = position2.Y;
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy, rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	de = dx ? dy/(float)dx : 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			defaultPerformDraw(tool, sim, brush, { y, x }, strength);
		else
			defaultPerformDraw(tool, sim, brush, { x, y }, strength);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(rx+ry) && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (reverseXY)
					defaultPerformDraw(tool, sim, brush, { y, x }, strength);
				else
					defaultPerformDraw(tool, sim, brush, { x, y }, strength);
			}
			e -= 1.0f;
		}
	}
}

void defaultPerformDrawRect(LuaTool *tool, Simulation *sim, Brush *brush, Point position1, Point position2, float strength)
{
	int x1 = position1.X, y1 = position1.Y;
	int x2 = position2.X, y2 = position2.Y;
	int brushX = ((x1 + x2) / 2);
	int brushY = ((y1 + y2) / 2);
	if (x1 > x2)
	{
		int temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		int temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (int j = y1; j <= y2; j++)
		for (int i = x1; i <= x2; i++)
			tool->CallPerform(sim, { i, j }, { brushX, brushY }, strength);
}

int LuaTool::DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength)
{
	if (luaToolRefs[index].draw)
	{
		luaDrawWrapper(index, sim, brush, position);
	}
	else
	{
		defaultPerformDraw(this, sim, brush, position, toolStrength);
	}

	return 0;
}

void LuaTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	if (luaToolRefs[index].drawLine)
	{
		luaDrawLineWrapper(index, sim, brush, startPos, endPos, held);
	}
	else
	{
		defaultPerformDrawLine(this, sim, brush, startPos, endPos, held, toolStrength);
	}
}

void LuaTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	if (luaToolRefs[index].drawRect)
	{
		luaDrawRectWrapper(index, sim, brush, startPos, endPos);
	}
	else
	{
		defaultPerformDrawRect(this, sim, brush, startPos, endPos, toolStrength);
	}
}

int LuaTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	if (luaToolRefs[index].drawFill)
	{
		luaDrawFillWrapper(index, sim, brush, position);
	}

	return 0;
}

void LuaTool::Click(Simulation *sim, Brush *brush, Point position)
{
	if (luaToolRefs[index].click)
	{
		luaClickWrapper(index, sim, brush, position);
	}
}

void LuaTool::Drag(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	if (luaToolRefs[index].drag)
	{
		luaDragWrapper(index, sim, brush, startPos, endPos);
	}
}

void LuaTool::Select(int toolIndex)
{
	if (luaToolRefs[index].select)
	{
		luaSelectWrapper(index, toolIndex);
	}
}
