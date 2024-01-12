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

int WARP_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp2 > 2000)
	{
		parts[i].temp = 10000;
		sim->air->pv[y/CELL][x/CELL] += (parts[i].tmp2/5000) * CFDS;
		if (RNG::Ref().chance(1, 50))
			sim->part_create(-3, x, y, PT_ELEC);
	}
	for (int trade = 0; trade < 5; trade ++)
	{
		int rx = RNG::Ref().between(-1, 1);
		int ry = RNG::Ref().between(-1, 1);
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r) != PT_WARP && TYP(r) != PT_STKM && TYP(r) != PT_STKM2 && !(sim->elements[TYP(r)].Properties&PROP_INDESTRUCTIBLE) && !(sim->elements[TYP(r)].Properties&PROP_CLONE))
			{
				parts[i].x = parts[ID(r)].x;
				parts[i].y = parts[ID(r)].y;
				parts[ID(r)].x = (float)x;
				parts[ID(r)].y = (float)y;
				parts[ID(r)].vx = RNG::Ref().between(0, 3) - 1.5f;
				parts[ID(r)].vy = RNG::Ref().between(0, 3) - 2.0f;
				parts[i].life += 4;
				pmap[y][x] = r;
				pmap[y+ry][x+rx] = PMAP(i, parts[i].type);
				trade = 5;
			}
		}
	}
	return 0;
}

int WARP_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = *colg = *colb = *cola = 0;
	*pixel_mode |= NO_DECO;
	return 0;
}

void WARP_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = RNG::Ref().between(70, 164);
}

void WARP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WARP";
	elem->Name = "WARP";
	elem->Colour = COLPACK(0x101010);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.8f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.9f;
	elem->Loss = 0.70f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 3.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 1;

	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->Description = "Displaces other elements. Completely invisible.";

	elem->Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WARP_update;
	elem->Graphics = &WARP_graphics;
	elem->Func_Create = &WARP_create;
	elem->Init = &WARP_init_element;
}
