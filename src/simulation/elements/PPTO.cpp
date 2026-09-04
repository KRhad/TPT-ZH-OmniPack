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

int PRTO_update(UPDATE_FUNC_ARGS);

int PPTO_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod;
	*firea = 8;
	*firer = 0;
	*fireg = 0;
	*fireb = 255;
	*pixel_mode |= EFFECT_GRAVOUT;
	*pixel_mode |= EFFECT_DBGLINES;
	lifemod = ((cpart->tmp2>10?10:cpart->tmp2)*20);
	*colb = 55 + lifemod;
	if (cpart->tmp2 < 10)
		*pixel_mode &= ~EFFECT_GRAVOUT;
	return 0;
}

void PPTO_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PPTO";
	elem->Name = "PPTO";
	elem->Colour = COLPACK(0x0020EB);
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
	elem->HotAir = 0.005f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "可通电开关的出口传送门。";
	elem->DetailedDescription = "描述：可控传送门出口，是 PRTO 的通电版本。PSCN 开启相连区域，NSCN 关闭；开启时从温度对应的频道释放 PPTI/PRTI 收集的物质、能量粒子和电脉冲，并产生轻微正压。\n频道：必须与入口保持相同温度才能互通。出口周围需要有空位；出口面积越大，可同时释放的粒子越多。没有可用粒子或没有空间时会等待。\n限制：关闭时不释放频道内容；不导热。\n导热率：0\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PRTO_update;
	elem->Graphics = &PPTO_graphics;
	elem->Init = &PPTO_init_element;
}
