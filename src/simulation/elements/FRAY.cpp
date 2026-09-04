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

int FRAY_update(UPDATE_FUNC_ARGS)
{
	int curlen;
	if (parts[i].tmp > 0)
		curlen = parts[i].tmp;
	else
		curlen = 10;
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_SPRK)
				{
					for (int nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1, len = 0; ; nyy+=nyi, nxx+=nxi, len++) {
						if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0) || len>curlen) {
							break;
						}
						r = pmap[y+nyi+nyy][x+nxi+nxx];
						if (!r)
							r = photons[y+nyi+nyy][x+nxi+nxx];
			
						if (r && !(sim->elements[TYP(r)].Properties & TYPE_SOLID))
						{
							auto coef = (parts[i].temp - 273.15f) / 10.0f;
							parts[ID(r)].vx = restrict_flt(parts[ID(r)].vx + nxi * coef, -MAX_VELOCITY, MAX_VELOCITY);
							parts[ID(r)].vy = restrict_flt(parts[ID(r)].vy + nyi * coef, -MAX_VELOCITY, MAX_VELOCITY);
						}
					}
				}
			}
	return 0;
}

void FRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FRAY";
	elem->Name = "FRAY";
	elem->Colour = COLPACK(0x00BBFF);
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 20.0f + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "力发射器，根据温度推拉物体。用法类似 ARAY。";
	elem->DetailedDescription = "描述：动力射线发射器(FRAY)通电后沿电流方向寻找可移动粒子并施加速度，方向判定与 ARAY 相同。目标温度高于 FRAY 时会被吸引，低于 FRAY 时会被推开；也能影响光子、中子等能量粒子。\n控制：FRAY 不导热，但可用升温笔(HEAT)和降温笔(COOL)设置自身温度，从而改变作用方向和强度。\n元素参数：Tmp 表示一次最多处理的粒子数；Tmp=0 时最多处理 10 个粒子。\n导热率：0\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FRAY_update;
	elem->Graphics = NULL;
	elem->Init = &FRAY_init_element;
}
