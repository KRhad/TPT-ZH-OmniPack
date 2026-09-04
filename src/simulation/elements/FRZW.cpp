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

int FRZW_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_WATR && RNG::Ref().chance(1, 14))
				{
					part_change_type(ID(r),x+rx,y+ry,PT_FRZW);
				}
			}
	if ((!parts[i].life && RNG::Ref().chance(1, 192)) || RNG::Ref().chance(100-parts[i].life, 50000))
	{
		part_change_type(i,x,y,PT_ICEI);
		parts[i].ctype=PT_FRZW;
		parts[i].temp = restrict_flt(parts[i].temp-200.0f, MIN_TEMP, MAX_TEMP);
	}
	return 0;
}

void FRZW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FRZW";
	elem->Name = "FRZW";
	elem->Colour = COLPACK(0x1020C0);
	elem->MenuVisible = 0;
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

	elem->DefaultProperties.temp = 120.0f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "冷冻水。冷冻粉熔化后形成的混合液体。";
	elem->DetailedDescription = "描述：寒尘(FRZZ)溶于水形成，能自身不断降温直到绝对零度，同时将更多的水变成寒水。\n凝固点：-220.15℃/53K\n导热率：29\n初始温度：-154.15℃";

	elem->Properties = TYPE_LIQUID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 53.0f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 100;

	elem->Update = &FRZW_update;
	elem->Graphics = NULL;
	elem->Init = &FRZW_init_element;
}
