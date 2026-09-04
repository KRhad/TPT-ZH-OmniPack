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

#ifndef NOMOD
#include "simulation/ElementsCommon.h"

int BUTN_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life >= 10)
	{
		*colr = 19;
		*colg = 229;
		*colb = 233;
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

bool BUTN_ctypeDraw(CTYPEDRAW_FUNC_ARGS)
{
	if (parts[i].life == 10 && t != PT_BUTN)
		sim->spark_conductive(i, parts[i].x, parts[i].y);
	return true;
}

void BUTN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BUTN";
	elem->Name = "BUTN";
	elem->Colour = COLPACK(0x005254);
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
	elem->HotAir = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "按钮。开启时可直接点击产生电火花。";
	elem->DetailedDescription = "描述：可通电的触发按钮。PSCN 使相连的 BUTN 开启，NSCN 关闭；开启时呈青色发光状态，并一直保持到收到关闭信号。\n用法：按钮开启后，在它上面绘制任意非 BUTN 元素，或让普通导体的电脉冲接触它，BUTN 会向电路输出一次 SPRK。适合在触屏上制作需要人工点击/绘制触发的电路。\n绝缘：INSL 和 RSSS 可阻隔按钮信号。\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &BUTN_graphics;
	elem->CtypeDraw = &BUTN_ctypeDraw;
	elem->Init = &BUTN_init_element;
}

#endif
