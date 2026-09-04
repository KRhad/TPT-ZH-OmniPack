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

int CO2_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
				{
					if (parts[i].ctype==5 && RNG::Ref().chance(1, 2000))
					{
						if (sim->part_create(-1, x+rx, y+ry, PT_WATR)>=0)
							parts[i].ctype = 0;
					}
					continue;
				}
				if (TYP(r)==PT_FIRE)
				{
					sim->part_kill(ID(r));
					if(RNG::Ref().chance(1, 30))
					{
						sim->part_kill(i);
						return 1;
					}
				}
				else if ((TYP(r)==PT_WATR || TYP(r)==PT_DSTW) && RNG::Ref().chance(1, 50))
				{
					part_change_type(ID(r), x+rx, y+ry, PT_CBNW);
					if (parts[i].ctype==5) //conserve number of water particles - ctype=5 means this CO2 hasn't released the water particle from BUBW yet
					{
						sim->part_create(i, x, y, PT_WATR);
						return 0;
					}
					else
					{
						sim->part_kill(i);
						return 1;
					}
				}
			}
	if (parts[i].temp > 9773.15 && sim->air->pv[y/CELL][x/CELL] > 200.0f)
	{
		if (RNG::Ref().chance(1, 5))
		{
			sim->part_create(i,x,y,PT_O2);

			int j = sim->part_create(-3,x,y,PT_NEUT);
			if (j != -1)
				parts[j].temp = MAX_TEMP;
			if (RNG::Ref().chance(1, 50))
			{
				j = sim->part_create(-3,x,y,PT_ELEC);
				if (j != -1)
					parts[j].temp = MAX_TEMP;
			}

			parts[i].temp = MAX_TEMP;
			sim->air->pv[y/CELL][x/CELL] += 100;
		}
	}
	return 0;
}

void CO2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CO2";
	elem->Name = "CO2";
	elem->Colour = COLPACK(0x666666);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 1.0f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->HeatConduct = 88;
	elem->Latent = 0;
	elem->Description = "二氧化碳。重气体，向下飘移。使水碳酸化并在寒冷时变成干冰。";
	elem->DetailedDescription = "描述：高密度气体。真空中会下沉。与水反应生成苏打水(BUBW)，低温下会变成干冰(DRIC)。不支持燃烧，可用于灭火。被植物(PLNT)吸收后形成氧气(OXYG)。\n产生：BUBW 会慢慢产生 CO2，会产生 0.5 P 的压力，随着压力的增加，产生的 CO2 减少。如果在密闭容器中，压力大于 3 P 时二氧化碳将停止产生。每一帧，BUBW 变化的几率是 4000 分之一。\nBUBW 在两种情况下几乎会立即爆炸：当它被任何粉末接触时，以及当它低于-5 P 压力时。每个粒子在爆炸时会释放 0.2 P 的压力。当它接触固体(不是 DMND 或 GLAS)时会产生更多的 CO2。对于附近的每个固体，它能够产生 40000 次CO2，并随之释放 0.2 P 的压力。BUBW 创建的所有 CO2 的 Ctype 值为 5，以将其标识为由它创建的。\n聚变：二氧化碳在高温(9500℃以上)、高压(200 P 以上)下会发生聚变，产生等离子体(PLSM)、极度高温高压的冲击波、一份中子(NEUT)、一份电子(ELEC)和一份氧气(OXYG)。\n凝固点：-78.5℃/194.65K\n导热率：88\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 194.65f;
	elem->LowTemperatureTransitionElement = PT_DRIC;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &CO2_update;
	elem->Graphics = NULL;
	elem->Init = &CO2_init_element;
}
