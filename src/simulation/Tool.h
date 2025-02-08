/**
 * Powder Toy - Tool (header)
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
#ifndef TOOL_H
#define TOOL_H

#include <string>
#include "defines.h"
#include "graphics/ARGBColour.h"
#include "simulation/StructProperty.h"


enum { ELEMENT_TOOL, WALL_TOOL, TOOL_TOOL, DECO_TOOL, GOL_TOOL, INVALID_TOOL, DECO_PRESET, FAV_MENU_BUTTON, HUD_MENU_BUTTON, CUSTOM_TOOL };

struct Point;
class Brush;
class Simulation;
class Tool
{
	std::string identifier;
	std::string name;
	std::string description;
	ARGBColour color;
	int MenuSection;
	int MenuVisible;

protected:
	int type;
	int toolID;
public:
	Tool(int toolID, std::string toolIdentifier, std::string name, std::string description, ARGBColour color, int MenuSection,
		 int MenuVisible = 1);
	Tool(int toolType, int toolID, std::string toolIdentifier, std::string name, std::string description, ARGBColour color, int MenuSection,
		 int MenuVisible = 1);
	virtual ~Tool() {}

	int GetType() { return type; }
	int GetID() { return toolID; }
	std::string GetIdentifier() { return identifier; }
	std::string GetName() { return name; }
	std::string GetDescription() { return description; }
	ARGBColour GetColor() { return color; }
	int GetMenuSection() { return MenuSection; }
	int GetMenuVisible() { return MenuVisible; }

	virtual int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength);
	virtual void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength);
	virtual void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength);
	virtual int FloodFill(Simulation *sim, Brush *brush, Point position);
	virtual void Click(Simulation *sim, Brush *brush, Point position);
	virtual void Drag(Simulation *sim, Brush *brush, Point startPos, Point endPos);
	virtual Tool * Sample(Simulation *sim, Point position, bool shiftHeld);
	virtual void Select(int toolIndex);
};

class ElementTool : public Tool
{
public:
	ElementTool(Simulation * sim, int elementID);
	int GetID();
};

class PlopTool : public ElementTool
{
public:
	PlopTool(Simulation * sim, int elementID);

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
	void Click(Simulation *sim, Brush *brush, Point position) override;
};

class GolTool : public Tool
{
public:
	GolTool(int golID);
	GolTool(int ruleset, std::string name, std::string description, ARGBColour color);
	int GetID();

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
};

class WallTool : public Tool
{
public:
	WallTool(int wallID);
	int GetID() { if (type == WALL_TOOL) return toolID; else return -1; }

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
};

class StreamlineTool : public WallTool
{
public:
	StreamlineTool();

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
};

class ToolTool : public Tool
{
public:
	ToolTool(int toolID, std::string name, ARGBColour color, std::string identifier, std::string description, int menuSection);
	ToolTool(int toolID);
	int GetID() { if (type == TOOL_TOOL) return toolID; else return -1; }

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
	void Click(Simulation *sim, Brush *brush, Point position) override;
};

class PropTool : public ToolTool
{
public:
	PropTool();
	~PropTool() override =default;

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
	Tool * Sample(Simulation *sim, Point position, bool shiftHeld) override;

	StructProperty prop;
	PropertyValue propValue;
	bool invalidState = true;
};

class DecoTool : public Tool
{
public:
	DecoTool(int decoID);
	int GetID() { if (type == DECO_TOOL) return toolID; else return -1; }

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override;
	Tool * Sample(Simulation *sim, Point position, bool shiftHeld) override;
};

class InvalidTool : public Tool
{
protected:
	InvalidTool(int type, int ID, std::string identifier, std::string name, std::string description, ARGBColour color, int menuSection);
public:
	int GetID() { return -1; }

	int DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength) override final;
	void DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength) override final;
	void DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos, float toolStrength) override final;
	int FloodFill(Simulation *sim, Brush *brush, Point position) override final;
	Tool * Sample(Simulation *sim, Point position, bool shiftHeld) override final;
};

class DecoPresetTool : public InvalidTool
{
public:
	DecoPresetTool(int presetID);
	int GetID() { if (type == DECO_PRESET) return toolID; else return -1; }
};

class FavTool : public InvalidTool
{
public:
	FavTool(int favID);
	int GetID() { if (type == FAV_MENU_BUTTON) return toolID; else return -1; }
};

class HudTool : public InvalidTool
{
public:
	HudTool(int hudID);
	int GetID() { if (type == HUD_MENU_BUTTON) return toolID; else return -1; }
};

#endif
