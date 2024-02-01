#include <sstream>
#include "Tool.h"
#include "defines.h"
#include "GolNumbers.h"
#include "graphics.h"
#include "hud.h"
#include "interface.h"
#include "powder.h"
#include "Simulation.h"
#include "ToolNumbers.h"
#include "WallNumbers.h"
#include "game/Brush.h"
#include "game/Menus.h"
#include "graphics/ARGBColour.h"
#include "interface/Engine.h"
#include "gui/gol/GolWindow.h"
#include "simulation/elements/LIFE.h"

Tool::Tool(int toolID, std::string toolIdentifier, std::string description):
	identifier(toolIdentifier),
	description(description),
	type(INVALID_TOOL),
	toolID(toolID)
{

}

Tool::Tool(int toolType, int toolID, std::string toolIdentifier, std::string description):
	identifier(toolIdentifier),
	description(description),
	type(toolType),
	toolID(toolID)
{

}

int Tool::DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength)
{
	return sim->CreateParts(position.X, position.Y, toolID, get_brush_flags(), true, brush);
}

void Tool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	if (held)
	{
#ifndef NOMOD
		if (toolID == PT_MOVS)
			return;
#endif
	}
	else
	{
		if (toolID == PT_LIGH)
			return;
	}
	sim->CreateLine(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID, get_brush_flags(), brush);
}

void Tool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	switch (toolID)
	{
	case PT_LIGH:
		return;
	case PT_TESC:
	{
		int radiusInfo = brush->GetRadius().X*4+brush->GetRadius().Y*4+7;
		sim->CreateBox(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID | PMAPID(radiusInfo), get_brush_flags());
		break;
	}
	default:
		sim->CreateBox(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID, get_brush_flags());
		break;
	}
}

int Tool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	switch (toolID)
	{
	case PT_LIGH:
#ifndef NOMOD
	case PT_MOVS:
#endif
		return 0;
	case PT_TESC:
	{
		int radiusInfo = brush->GetRadius().X*4+brush->GetRadius().Y*4+7;
		return sim->FloodParts(position.X, position.Y, toolID | PMAPID(radiusInfo), -1, get_brush_flags());
	}
	default:
		return sim->FloodParts(position.X, position.Y, toolID, -1, get_brush_flags());
	}
}

void Tool::Click(Simulation *sim, Point position)
{

}

Tool * Tool::Sample(Simulation *sim, Point position, bool shiftHeld)
{
	if (position.Y < 0 || position.Y >= YRES || position.X < 0 || position.X >= XRES)
		return this;

	int sample = pmap[position.Y][position.X];
	if (sample || (sample = photons[position.Y][position.X]))
	{
		if (TYP(sample) == PT_LIFE)
		{
			int ctype = parts[ID(sample)].ctype;
			if (ctype < NGOL)
				return GetToolFromIdentifier("DEFAULT_PT_LIFE_" + builtinGol[ctype].name);
			else
			{
				for (auto *tool : menuSections[SC_LIFE]->tools)
				{
					if  (tool->GetID() == ctype)
						return tool;
				}
				std::string ruleStr = SerialiseGOLRule(ctype);
				auto *golWindow = new GolWindow(ruleStr, parts[ID(sample)].dcolour, parts[ID(sample)].tmp);
				Engine::Ref().ShowWindow(golWindow);
			}
		}
		else
		{
			return GetToolFromIdentifier(sim->elements[TYP(sample)].Identifier);
		}
	}
	else if (bmap[position.Y/CELL][position.X/CELL] > 0 && bmap[position.Y/CELL][position.X/CELL] < WALLCOUNT)
	{
		return GetToolFromIdentifier(wallTypes[bmap[position.Y / CELL][position.X / CELL]].identifier);
	}
	return this;
}


ElementTool::ElementTool(Simulation * sim, int elementID):
	Tool(ELEMENT_TOOL, elementID, sim->elements[elementID].Identifier, sim->elements[elementID].Description)
{

}
int ElementTool::GetID()
{
	if (type == ELEMENT_TOOL)
		return toolID;
	else if (type == GOL_TOOL)
		return PT_LIFE;
	else
		return -1;
}


PlopTool::PlopTool(Simulation * sim, int elementID):
	ElementTool(sim, elementID)
{

}
int PlopTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	return 0;
}
void PlopTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{

}

void PlopTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{

}

int PlopTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	return 0;
}
void PlopTool::Click(Simulation *sim, Point position)
{
	sim->part_create(-1, position.X, position.Y, toolID);
}


GolTool::GolTool(int golID):
	Tool(GOL_TOOL, golID, "DEFAULT_PT_LIFE_" + builtinGol[golID].name, builtinGol[golID].description)
{

}
GolTool::GolTool(int ruleset, std::string name, std::string description):
	Tool(GOL_TOOL, ruleset, "DEFAULT_PT_LIFECUST_" + name, description)
{

}
int GolTool::GetID()
{
	if (type == GOL_TOOL)
		return toolID;
	//else if (type == ELEMENT_TOOL)
	//	return PT_LIFE;
	else
		return -1;
}
int GolTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	return sim->CreateParts(position.X, position.Y, PT_LIFE | PMAPID(toolID), get_brush_flags(), true, brush);
}
void GolTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	sim->CreateLine(startPos.X, startPos.Y, endPos.X, endPos.Y, PT_LIFE | PMAPID(toolID), get_brush_flags(), brush);
}
void GolTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	sim->CreateBox(startPos.X, startPos.Y, endPos.X, endPos.Y, PT_LIFE | PMAPID(toolID), get_brush_flags());
}
int GolTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	return sim->FloodParts(position.X, position.Y, PT_LIFE | PMAPID(toolID), -1, get_brush_flags());
}


WallTool::WallTool(int wallID):
	Tool(WALL_TOOL, wallID, wallTypes[wallID].identifier, wallTypes[wallID].descs)
{

}
int WallTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	int rx = brush->GetRadius().X/CELL;
	int ry = brush->GetRadius().Y/CELL;
	int x = position.X/CELL;
	int y = position.Y/CELL;
	sim->CreateWallBox(x-rx, y-ry, x+rx, y+ry, toolID);
	return 1;
}
void WallTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	if (!held && toolID == WL_FAN && bmap[startPos.Y/CELL][startPos.X/CELL] == WL_FAN)
	{
		float nfvx = (endPos.X-startPos.X)*0.005f;
		float nfvy = (endPos.Y-startPos.Y)*0.005f;
		sim->FloodWalls(startPos.X/CELL, startPos.Y/CELL, WL_FANHELPER, WL_FAN);
		for (int j=0; j<YRES/CELL; j++)
			for (int i=0; i<XRES/CELL; i++)
				if (bmap[j][i] == WL_FANHELPER)
				{
					sim->air->fvx[j][i] = nfvx;
					sim->air->fvy[j][i] = nfvy;
					bmap[j][i] = WL_FAN;
				}
	}
	else
	{
		sim->CreateWallLine(startPos.X/CELL, startPos.Y/CELL, endPos.X/CELL, endPos.Y/CELL, brush->GetRadius().X/CELL, brush->GetRadius().Y/CELL, toolID);
	}
}
void WallTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	sim->CreateWallBox(startPos.X/CELL, startPos.Y/CELL, endPos.X/CELL, endPos.Y/CELL, toolID);
}
int WallTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	return sim->FloodWalls(position.X/CELL, position.Y/CELL, toolID, -1);
}

StreamlineTool::StreamlineTool():
	WallTool(WL_STREAM)
{

}
int StreamlineTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	sim->CreateWall(position.X, position.Y, toolID);
	return 0;
}
void StreamlineTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	sim->CreateWallLine(startPos.X/CELL, startPos.Y/CELL, endPos.X/CELL, endPos.Y/CELL, 0, 0, toolID);
}
int StreamlineTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	return 0;
}

ToolTool::ToolTool(int toolID):
	Tool(TOOL_TOOL, toolID, toolTypes[toolID].identifier, toolTypes[toolID].descs)
{

}
int ToolTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	if (toolID == TOOL_SIGN || toolID == TOOL_GOL)
		return 1;
	sim->CreateToolBrush(position.X, position.Y, toolID, toolStrength, brush);
	return 0;
}
void ToolTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	if (toolID == TOOL_SIGN || toolID == TOOL_GOL)
		return;
	if (toolID == TOOL_WIND)
		toolStrength = held ? 0.01f*toolStrength : 0.002f;
	sim->CreateToolLine(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID, toolStrength, brush);
}
void ToolTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	if (toolID == TOOL_SIGN || toolID == TOOL_GOL)
		return;
	sim->CreateToolBox(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID, toolStrength);
}
int ToolTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	return 0;
}
void ToolTool::Click(Simulation *sim, Point position)
{
	if (toolID == TOOL_SIGN)
	{
		openSign = true;
	}
}

PropTool::PropTool():
	ToolTool(TOOL_PROP)
{
	propValue.Integer = 0;
}
int PropTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	if (!invalidState)
		sim->CreatePropBrush(position.X, position.Y, prop, propValue, brush);
	return 0;
}
void PropTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	if (!invalidState)
		sim->CreatePropLine(startPos.X, startPos.Y, endPos.X, endPos.Y, prop, propValue, brush);
}
void PropTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	if (!invalidState)
		sim->CreatePropBox(startPos.X, startPos.Y, endPos.X, endPos.Y, prop, propValue);
}
int PropTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	if (!invalidState)
		return sim->FloodProp(position.X, position.Y, prop, propValue);
	return 0;
}

Tool * PropTool::Sample(Simulation *sim, Point position, bool shiftHeld)
{
	if (!shiftHeld)
		return Tool::Sample(sim, position, shiftHeld);

	if (position.Y < 0 || position.Y >= YRES || position.X < 0 || position.X >= XRES)
		return this;

	int sample = pmap[position.Y][position.X];
	if (sample || (sample = photons[position.Y][position.X]))
	{
		particle part = parts[ID(sample)];
		switch (prop.Type)
		{
			case StructProperty::Float:
			{
				float value = *((float*)(((char*)&part) + prop.Offset));
				propValue.Float = value;
				break;
			}
			case StructProperty::ParticleType:
			case StructProperty::Integer:
			{
				int value = *((int*)(((char*)&part) + prop.Offset));
				propValue.Integer = value;
				break;
			}
			case StructProperty::UInteger:
			{
				unsigned int value = *((unsigned int*)(((char*)&part) + prop.Offset));
				propValue.UInteger = value;
				break;
			}
			default:
				break;
		}

		openProp = true; // tell main window to open the prop tool
	}
	return this;
}

DecoTool::DecoTool(int decoID):
	Tool(DECO_TOOL, decoID, decoTypes[decoID].identifier, decoTypes[decoID].descs)
{

}
int DecoTool::DrawPoint(Simulation *sim, Brush* brush, Point position, float toolStrength)
{
	ARGBColour col = (toolID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolor;
	sim->CreateDecoBrush(position.X, position.Y, toolID, col, brush);
	return 0;
}
void DecoTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{
	ARGBColour col = (toolID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolor;
	sim->CreateDecoLine(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID, col, brush);
}
void DecoTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{
	ARGBColour col = (toolID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolor;
	sim->CreateDecoBox(startPos.X, startPos.Y, endPos.X, endPos.Y, toolID, col);
}
int DecoTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	pixel rep = vid_buf[position.X+position.Y*(XRES+BARSIZE)];
	unsigned int col = (toolID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolor;
	sim->FloodDeco(vid_buf, position.X, position.Y, col, PIXCONV(rep));
	return 1;
}
Tool * DecoTool::Sample(Simulation *sim, Point position, bool shiftHeld)
{
	if (position.Y < 0 || position.Y >= YRES || position.X < 0 || position.X >= XRES)
		return this;

	int cr = PIXR(sampleColor);
	int cg = PIXG(sampleColor);
	int cb = PIXB(sampleColor);

	decocolor = COLARGB(255, cr, cg, cb);
	currR = cr, currG = cg, currB = cb, currA = 255;
	RGB_to_HSV(currR, currG, currB, &currH, &currS, &currV);
	return this;
}

InvalidTool::InvalidTool(int type, int toolID, std::string identifier, std::string description):
	Tool(type, toolID, identifier, description)
{

}
int InvalidTool::DrawPoint(Simulation *sim, Brush *brush, Point position, float toolStrength)
{
	return 0;
}
void InvalidTool::DrawLine(Simulation *sim, Brush *brush, Point startPos, Point endPos, bool held, float toolStrength)
{

}
void InvalidTool::DrawRect(Simulation *sim, Brush *brush, Point startPos, Point endPos)
{

}
int InvalidTool::FloodFill(Simulation *sim, Brush *brush, Point position)
{
	return 0;
}
Tool * InvalidTool::Sample(Simulation *sim, Point position, bool shiftHeld)
{
	return nullptr;
}

DecoPresetTool::DecoPresetTool(int decoPresetID):
InvalidTool(DECO_PRESET, decoPresetID, colorlist[decoPresetID].identifier, colorlist[decoPresetID].descs)
{

}

FavTool::FavTool(int favID):
	InvalidTool(FAV_MENU_BUTTON, favID, fav[favID].identifier, fav[favID].description)
{

}

HudTool::HudTool(int hudID):
InvalidTool(HUD_MENU_BUTTON, hudID, "DEFAULT_HUD_" + hud_menu[hudID].name, hud_menu[hudID].description)
{

}
