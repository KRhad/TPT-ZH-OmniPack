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

int SHLD2_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
				{
					if (parts[i].life > 0)
						sim->part_create(-1,x+rx,y+ry,PT_SHLD1);
					continue;
				}
				else if (TYP(r)==PT_SPRK && !parts[i].life)
				{
					if (RNG::Ref().chance(1, 8))
					{
						part_change_type(i,x,y,PT_SHLD3);
						parts[i].life = 7;
					}
					for (int nnx = -1; nnx <= 1; nnx++)
						for (int nny = -1; nny <= 1; nny++)
						{
							if (!pmap[y+ry+nny][x+rx+nnx])
							{
								int np = sim->part_create(-1,x+rx+nnx,y+ry+nny,PT_SHLD1);
								if (np<0) continue;
								parts[np].life=7;
							}
						}
				}
				else if (TYP(r)==PT_SHLD4 && RNG::Ref().chance(2, 5))
				{
					part_change_type(i,x,y,PT_SHLD3);
					parts[i].life = 7;
				}
			}
	return 0;
}

void SHLD2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SHLD2";
	elem->Name = "SHD2";
	elem->Colour = COLPACK(0x777777);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
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

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "护盾2级。";
	elem->DetailedDescription = "描述：通电时，会自动生长出保护膜，从内到外依次是 SHD4、SHD3、SHD2、SHLD。除 SHLD 外的所有类型都将用下面的级别填充周围的空白空间。例如，SHD3 将用 SHD2 包围自己。分解压力分别为：40/25/15/7 P。不导电，不导热。\n导热率：0/0/0/0\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 15.0f;
	elem->HighPressureTransitionElement = PT_NONE;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &SHLD2_update;
	elem->Graphics = NULL;
	elem->Init = &SHLD2_init_element;
}
