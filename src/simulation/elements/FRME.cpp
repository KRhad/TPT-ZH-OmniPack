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

int FRME_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->tmp)
	{
		*colr += 30;
		*colg += 30;
		*colb += 30;
	}
	return 0;
}
void FRME_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FRME";
	elem->Name = "FRME";
	elem->Colour = COLPACK(0x999988);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "框架，可以与活塞一起使用来推动许多颗粒。";
	elem->DetailedDescription = "描述：用于增加活塞一次性推动物质的数量，至少需要1 像素厚度，最多能向一个方向延长15 像素，用活塞(PSTN)推动其中一个像素就可以推动和收回整个支架(以及支架上方的物质)。如果某一个像素的支架被挡住(比如墙)，那么整个支架都不会移动,任何被 FRME 捕获的粒子都会阻止整个事物缩回。确保保持框架后面的路径畅通。\n注意事项：只有位于活塞上方第一层的支架能起作用，第二层之后的支架是不起支撑作用的(也就是说你不能建造一个树杈状的支架并整体移动它)。同时，如果你使用了两个以上的活塞来推动支架，那么在收回时它们会互相挡住。令 Tmp=1 可以使支架变为“非粘性”，也就是说此时支架被推出后就不能被收回。\n导热率：0\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &FRME_graphics;
	elem->Init = &FRME_init_element;
}
