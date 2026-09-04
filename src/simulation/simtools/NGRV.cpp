#include "simulation/ToolCommon.h"

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushYy, float strength);

void SimTool::Tool_NGRV()
{
	Identifier = "DEFAULT_TOOL_NGRV";
	Name = "NGRV";
	Colour = 0xAACCFF_rgb;
	Description = "创建一个短暂的负重力井。";
	Perform = &perform;
}

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushYy, float strength)
{
	sim->gravIn.mass[Vec2{ x, y } / CELL] = strength * -5.0f;
	return 1;
}
