#ifndef LUATOOL_H
#define LUATOOL_H

#include <map>
#include "LuaSmartRef.h"
#include "simulation/Tool.h"

struct CustomTool
{
	LuaSmartRef perform;
	LuaSmartRef click;
	LuaSmartRef drag;
	LuaSmartRef draw;
	LuaSmartRef drawLine;
	LuaSmartRef drawRect;
	LuaSmartRef drawFill;
	LuaSmartRef select;
};

class particle;

class LuaTool : public Tool
{
	int index;

public:
	LuaTool(int index, std::string name, ARGBColour color, std::string identifier, std::string description, int menuSection);

	void CallPerform(Simulation *sim, Point position, Point brushOffset, float strength);

	int luaPerformWrapper(LuaTool *tool, Simulation *sim, particle *cpart, int x, int y, int brushX, int brushY, float strength);
	void luaClickWrapper(int index, Simulation *sim, Brush *brush, Point position);
	void luaDragWrapper(int index, Simulation *sim, Brush *brush, Point position1, Point position2);
	void luaDrawWrapper(int index, Simulation *sim, Brush *brush, Point position);;
	void luaDrawLineWrapper(int index, Simulation *sim, Brush *brush, Point position1, Point position2, bool dragging);
	void luaDrawRectWrapper(int index, Simulation *sim, Brush *brush, Point position1, Point position2);
	void luaDrawFillWrapper(int index, Simulation *sim, Brush *brush, Point position);
	void luaSelectWrapper(int index, int toolSelection);

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
	void Click(Simulation *sim, Brush *brush, Point position) override;
	void Drag(Simulation *sim, Brush *brush, Point startPos, Point endPos) override;
	void Select(int toolIndex) override;

	int GetIndex() { return index; }
};

struct LuaToolData
{
	int index;
	std::string name;
	ARGBColour color;
	std::string identifier;
	std::string description;
	int menuSection;
	int menuVisible;

	LuaToolData() = default;
	LuaToolData(int index, std::string name, ARGBColour color, std::string identifier, std::string description, int menuSection,
				int menuVisible):
		index(index),
		name(name),
		color(color),
		identifier(identifier),
		description(description),
		menuSection(menuSection),
		menuVisible(menuVisible)
	{ }

	LuaTool *GetTool() const
	{
		return new LuaTool(index, name, color, identifier, description, menuSection);
	}
};

extern std::map<int, LuaToolData> luaTools;
extern std::map<int, CustomTool> luaToolRefs;

#endif // LUATOOL_H
