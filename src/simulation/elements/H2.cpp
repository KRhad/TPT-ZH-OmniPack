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
	elem->Description = "Hydrogen. Combusts with OXYG to make WATR. Undergoes fusion at high temperature and pressure.";

	elem->Properties = TYPE_GAS;

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
