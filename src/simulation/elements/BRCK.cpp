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

int BRCK_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp == 1)
	{
        *pixel_mode |= FIRE_ADD;
        *colb += 100;

        *firea = 40;
        *firer = *colr;
        *fireg = *colg;
        *fireb = *colb;
	}
	return 0;
}

void BRCK_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BRCK";
	elem->Name = "BRCK";
	elem->Colour = COLPACK(0x808080);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "砖，易碎的建筑材料。";
	elem->DetailedDescription = "描述：可破坏的建筑材料。是石粉的固体形式，不能导电，可以熔化。在压力大于8.8 P 时会碎裂成石粉(STNE)。\n元素参数：修改其 Tmp 值为 1 可以制得像可控动力管(PPIP)那样的蓝光砖块。\n制取方法：浆糊(PSTE)加热至 480℃/753.15K 可以转化成砖块。\n压力极限：8.8 P\n熔点：949.85℃/1223K\n导热率：251\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 8.8f;
	elem->HighPressureTransitionElement = PT_STNE;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1223.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = NULL;
	elem->Graphics = &BRCK_graphics;
	elem->Init = &BRCK_init_element;
}
