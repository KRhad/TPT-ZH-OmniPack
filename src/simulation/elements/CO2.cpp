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
	elem->Description = "Carbon Dioxide. Heavy gas, drifts downwards. Carbonates water and turns to dry ice when cold.";

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
