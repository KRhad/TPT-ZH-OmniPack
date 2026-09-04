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

int PRTI_update(UPDATE_FUNC_ARGS);

int PPTI_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod;
	*firea = 8;
	*firer = 255;
	*fireg = 0;
	*fireb = 0;
	*pixel_mode |= EFFECT_GRAVIN;
	*pixel_mode |= EFFECT_DBGLINES;
	lifemod = ((cpart->tmp2>10?10:cpart->tmp2)*10);
	*colr = 155 + lifemod;
	if (cpart->tmp2 < 10)
		*pixel_mode &= ~EFFECT_GRAVIN;
	return 0;
}

void PPTI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PPTI";
	elem->Name = "PPTI";
	elem->Colour = COLPACK(0xEB5917);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = -0.005f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "可通电开关的入口传送门。";
	elem->DetailedDescription = "描述：可控传送门入口，是 PRTI 的通电版本。PSCN 开启相连区域，NSCN 关闭；开启时产生轻微负压，把接触的物质、能量粒子和电脉冲存入对应频道，等待出口释放。\n频道：频道由温度决定，只有温度对应的 PPTI/PRTI 与 PPTO/PRTO 才互通。入口表面积越大，吸收速度越高；频道暂时没有出口时可保存有限数量的粒子。\n限制：关闭时不执行入口传送；不导热。\n导热率：0\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PRTI_update;
	elem->Graphics = &PPTI_graphics;
	elem->Init = &PPTI_init_element;
}
