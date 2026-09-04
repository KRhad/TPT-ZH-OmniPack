#include "simulation/ToolCommon.h"

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength);

void SimTool::Tool_PGRV()
{
	Identifier = "DEFAULT_TOOL_PGRV";
	Name = "PGRV";
	Colour = 0xCCCCFF_rgb;
	Description = "创建一个短暂的重力井。";
	Perform = &perform;
}

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	sim->gravIn.mass[Vec2{ x, y } / CELL] = strength * 5.0f;
	return 1;
}
