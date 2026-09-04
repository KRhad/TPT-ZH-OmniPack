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

int H2_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				if (sim->air->pv[y/CELL][x/CELL] > 8.0f && rt == PT_DESL) // This will not work. DESL turns to fire above 5.0 pressure
				{
					part_change_type(ID(r),x+rx,y+ry,PT_WATR);
					part_change_type(i,x,y,PT_OIL);
					return 1;
				}
				if (sim->air->pv[y/CELL][x/CELL] > 45.0f)
				{
					if (parts[ID(r)].temp > 2273.15)
						continue;
				}
				else
				{
					if (rt==PT_FIRE)
					{
						if(parts[ID(r)].tmp&0x02)
							parts[ID(r)].temp = 3473;
						else
							parts[ID(r)].temp = 2473.15f;
						parts[ID(r)].tmp |= 1;

						sim->part_create(i,x,y,PT_FIRE);
						parts[i].temp += RNG::Ref().between(0, 99);
						parts[i].tmp |= 1;
						return 1;
					}
					else if ((rt==PT_PLSM && !(parts[ID(r)].tmp&4)) || (rt==PT_LAVA && parts[ID(r)].ctype != PT_BMTL))
					{
						sim->part_create(i,x,y,PT_FIRE);
						parts[i].temp += RNG::Ref().between(0, 99);
						parts[i].tmp |= 1;
						sim->air->pv[y/CELL][x/CELL] += 0.1f;
						return 1;
					}
				}
			}
	if (parts[i].temp > 2273.15f && sim->air->pv[y/CELL][x/CELL] > 50.0f)
	{
		if (RNG::Ref().chance(1, 5))
		{
			int j;
			float temp = parts[i].temp;
			sim->part_create(i,x,y,PT_NBLE);
			parts[i].tmp = 0x1;

			j = sim->part_create(-3,x,y,PT_NEUT);
			if (j > -1)
				parts[j].temp = temp;
			if (RNG::Ref().chance(1, 10))
			{
				j = sim->part_create(-3,x,y,PT_ELEC);
				if (j > -1)
					parts[j].temp = temp;
			}
			j = sim->part_create(-3,x,y,PT_PHOT);
			if (j >-1)
			{
				parts[j].ctype = 0x7C0000;
				parts[j].temp = temp;
				parts[j].tmp = 0x1;
			}
			int rx = x + RNG::Ref().between(-1, 1), ry = y + RNG::Ref().between(-1, 1), rt = TYP(pmap[ry][rx]);
			if (sim->can_move[PT_PLSM][rt] || rt == PT_H2)
			{
				j = sim->part_create(-3,rx,ry,PT_PLSM);
				if (j > -1)
				{
					parts[j].temp = temp;
					parts[j].tmp |= 4;
				}
			}

			parts[i].temp = temp + 750 + RNG::Ref().between(0, 499);
			sim->air->pv[y/CELL][x/CELL] += 30;
			return 1;
		}
	}
	return 0;
}

void H2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_H2";
	elem->Name = "HYGN";
	elem->Colour = COLPACK(0x5070FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.10f;
	elem->Gravity = 0.00f;
	elem->Diffusion = 3.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "氢气。与 OXYG 燃烧生成 WATR；在高温高压下发生聚变。";
	elem->DetailedDescription = "描述：氢气(HYGN)可被火焰(FIRE)点燃，并与氧气(OXYG)燃烧生成水蒸气(WTRV)。它自身不产生气压，因此在低温下可接触石英(QRTZ)而不使石英因压力破碎。\n与柴油的反应：HYGN 压力大于 8 P 且接触 DESL 时，两者分别转化为 OIL 和 WATR。DESL 在压力超过 5 P 时会先变成 FIRE，因此需要迅速完成反应，或用 TTAN 隔绝 DESL 所受压力，只给 HYGN 加压。\n聚变：约 2000℃、50 P 时，HYGN 可聚变为惰性气体(NBLE)，同时产生 PLSM、NEUT、黄色 PHOT，以及 1 至 2 个 NBLE；另有 10% 概率产生 ELEC。反应会释放约 50 P 压力并把温度提高到约 4000℃。\n产生：NEUT+ELEC→HYGN。\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_GAS | PROP_PHOTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &H2_update;
	elem->Graphics = NULL;
	elem->Init = &H2_init_element;
}
