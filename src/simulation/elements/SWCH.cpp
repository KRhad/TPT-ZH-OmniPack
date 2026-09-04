/*
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

#include "simulation/ElementsCommon.h"

bool isRedBRAY(UPDATE_FUNC_ARGS, int xc, int yc)
{
	return TYP(pmap[yc][xc]) == PT_BRAY && parts[ID(pmap[yc][xc])].tmp == 2;
}

int SWCH_update(UPDATE_FUNC_ARGS)
{
	// Turn SWCH on/off from two red BRAYS. There must be one either above or below, and one either left or right to work, and it can't come from the side, it must be a diagonal beam
	if (!TYP(pmap[y-1][x-1]) && !TYP(pmap[y-1][x+1])
	        && (isRedBRAY(UPDATE_FUNC_SUBCALL_ARGS, x, y - 1) || isRedBRAY(UPDATE_FUNC_SUBCALL_ARGS, x, y + 1))
	        && (isRedBRAY(UPDATE_FUNC_SUBCALL_ARGS, x + 1, y) || isRedBRAY(UPDATE_FUNC_SUBCALL_ARGS, x - 1, y)))
	{
		if (parts[i].life == 10)
			parts[i].life = 9;
		else if (parts[i].life <= 5)
			parts[i].life = 14;
	}
	return 0;
}

int SWCH_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life >= 10)
	{
		*colr = 17;
		*colg = 217;
		*colb = 24;
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void SWCH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SWCH";
	elem->Name = "SWCH";
	elem->Colour = COLPACK(0x103B11);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "开关。仅在开启时导电（PSCN 开启，NSCN 关闭）。";
	elem->DetailedDescription = "描述：从 PSCN 导入电时，开关可导电。从NSCN 导入电时，开关不可导电。SWCH 关闭时，是暗绿色，开启时，是绿色。通过装饰功能，开关可制作实用的电灯泡。它导电的速度，与从哪儿导入电有关，这是一个粒子顺序的话题。在开始导电时，它的导电速度就保存下来，从左上角导入，则它的导电速度更快，其它方向导入，则偏慢一些。\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &SWCH_update;
	elem->Graphics = &SWCH_graphics;
	elem->Init = &SWCH_init_element;
}
