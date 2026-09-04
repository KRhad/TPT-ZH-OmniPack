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

int CBNW_update(UPDATE_FUNC_ARGS)
{
	if (sim->air->pv[y/CELL][x/CELL] <= 3)
	{
		if (sim->air->pv[y/CELL][x/CELL] <= -0.5 || RNG::Ref().chance(1, 4000))
		{
			part_change_type(i, x, y, PT_CO2);
			parts[i].ctype = 5;
			sim->air->pv[y/CELL][x/CELL] += 0.5f;
		}
	}
	if (parts[i].tmp2!=20)
	{
		parts[i].tmp2 -= (parts[i].tmp2>20)?1:-1;
	}
	else if (RNG::Ref().chance(1, 200))
	{
		parts[i].tmp2 = RNG::Ref().between(0, 39);
	}

	if (parts[i].tmp > 0)
	{
		// Explode
		if (parts[i].tmp == 1 && RNG::Ref().chance(3, 4))
		{
			part_change_type(i, x, y, PT_CO2);
			parts[i].ctype = 5;
			sim->air->pv[y/CELL][x/CELL] += 0.2f;
		}
		parts[i].tmp--;
	}
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				if (sim->elements[rt].Properties&TYPE_PART && parts[i].tmp == 0 && RNG::Ref().chance(1, 83))
				{
					//Start explode
					parts[i].tmp = RNG::Ref().between(0, 24);
				}
				else if (sim->elements[rt].Properties&TYPE_SOLID && !(sim->elements[rt].Properties&PROP_INDESTRUCTIBLE)
				        && rt != PT_GLAS && parts[i].tmp == 0 && RNG::Ref().chance((2 - sim->air->pv[y/CELL][x/CELL]), 6667))
				{
					if (RNG::Ref().chance(1, 2))
					{
						part_change_type(i, x, y, PT_CO2);
						parts[i].ctype = 5;
						sim->air->pv[y/CELL][x/CELL] += 0.2f;
					}
				}
				if (rt == PT_CBNW)
				{
					if (!parts[i].tmp)
					{
						if (parts[ID(r)].tmp)
						{
							parts[i].tmp = parts[ID(r)].tmp;
							// If the other particle hasn't been life updated
							if ((ID(r)) > i)
								parts[i].tmp--;
						}
					}
					else if (!parts[ID(r)].tmp)
					{
						parts[ID(r)].tmp = parts[i].tmp;
						// If the other particle hasn't been life updated
						if ((ID(r)) > i)
							parts[ID(r)].tmp++;
					}
				}
				else if (rt == PT_RBDM || rt == PT_LRBD)
				{
					if ((legacy_enable || parts[i].temp > (273.15f + 12.0f)) && RNG::Ref().chance(1, 166))
					{
						part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 4;
						parts[i].ctype = PT_WATR;
					}
				}
				else if (rt == PT_FIRE && parts[ID(r)].ctype != PT_WATR)
				{
					sim->part_kill(ID(r));
					if (RNG::Ref().chance(1, 50))
					{
						sim->part_kill(i);
						return 1;
					}
				}
			}
	return 0;
}

int CBNW_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = cpart->tmp2 - 20;//speckles!
	*colr += z * 1;
	*colg += z * 2;
	*colb += z * 8;
	return 0;
}

void CBNW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CBNW";
	elem->Name = "BUBW";
	elem->Colour = COLPACK(0x2030D0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 7500;
	elem->Description = "碳酸水。慢慢释放CO2。";
	elem->DetailedDescription = "描述：和其他物质接触时会释放出二氧化碳(CO2)并产生压力。\n产生：BUBW 会慢慢产生 CO2，会产生 0.5 P 的压力，随着压力的增加，产生的 CO2 减少。如果在密闭容器中，压力大于 3 P 时二氧化碳将停止产生。每一帧，BUBW 变化的几率是 4000 分之一。\nBUBW 在两种情况下几乎会立即爆炸：当它被任何粉末接触时，以及当它低于-5 P 压力时。每个粒子在爆炸时会释放 0.2 P 的压力。当它接触固体(不是 DMND 或 GLAS)时会产生更多的 CO2。对于附近的每个固体，它有 1/40000 的概率产生 CO2，并随之释放 0.2 P 的压力。BUBW 创建的所有 CO2 的 Ctype 值为 5，以将其标识为是由它创建的。\n沸点：99.85℃/373.0K\n凝固点：0℃/273.15K\n导热率：29\n初始温度：20.00℃/293.15K";

	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 273.15f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &CBNW_update;
	elem->Graphics = &CBNW_graphics;
	elem->Init = &CBNW_init_element;
}
