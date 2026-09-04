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

int YEST_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_DYST && RNG::Ref().chance(1, 6) && !legacy_enable)
				{
					part_change_type(i,x,y,PT_DYST);
				}
			}
	if (parts[i].temp>303 && parts[i].temp<317)
	{
		sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), PT_YEST);
	}
	return 0;
}

void YEST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_YEST";
	elem->Name = "YEST";
	elem->Colour = COLPACK(0xEEE0C0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 15;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 31;

	elem->Weight = 80;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "酵母在温暖（~37C）时生长。";
	elem->DetailedDescription = "描述：酵母，在特定温度范围(29.85℃/303K~43.85℃/317K,不包括边界值)会繁殖。被中子(NEUT)轰击或者温度太高(99.85℃/373K 以上)会死掉变成菌尸(DYST)。菌尸在更高温度(199.85℃/473K 以上)下会变成尘埃(DUST)。在任何温度下，酵母(YEST)触碰到菌尸(DYST)都会死亡。\n元素参数：菌尸(DYST)可以燃烧 20 帧(暂停时修改火焰温度即可点燃)\n导热率：70/70\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_DYST;

	elem->Update = &YEST_update;
	elem->Graphics = NULL;
	elem->Init = &YEST_init_element;
}
