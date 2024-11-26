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

int C5_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((TYP(r)!=PT_C5 && parts[ID(r)].temp<100 && sim->elements[TYP(r)].HeatConduct && (TYP(r)!=PT_HSWC||parts[ID(r)].life==10)) || TYP(r)==PT_HFLM)
				{
					if (RNG::Ref().chance(1, 6))
					{
						sim->part_change_type(i,x,y,PT_HFLM);
						parts[ID(r)].temp = parts[i].temp = 0;
						parts[i].life = RNG::Ref().between(50, 199);
						sim->air->pv[y/CELL][x/CELL] += 1.5;
					}
				}
			}
	if (parts[i].ctype && !parts[i].life)
	{
		float vx = ((parts[i].tmp << 16) >> 16) / 255.0f;
		float vy = (parts[i].tmp >> 16) / 255.0f;
		float dx = ((parts[i].tmp2 << 16) >> 16) / 255.0f;
		float dy = (parts[i].tmp2 >> 16) / 255.0f;
		int r = sim->part_create(-3, x, y, PT_PHOT);
		if (r != -1)
		{
			parts[r].ctype = parts[i].ctype;
			parts[r].x += dx;
			parts[r].y += dy;
			parts[r].vx = vx;
			parts[r].vy = vy;
			parts[r].temp = parts[i].temp;
		}
		parts[i].ctype = 0;
		parts[i].tmp = 0;
		parts[i].tmp2 = 0;
	}
	return 0;
}

int C5_graphics(GRAPHICS_FUNC_ARGS)
{
	if (!cpart->ctype)
		return 0;

	int x = 0;
	*colr = *colg = *colb = 0;
	for (x=0; x<12; x++) {
		*colr += (cpart->ctype >> (x+18)) & 1;
		*colb += (cpart->ctype >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (cpart->ctype >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	*colr *= x;
	*colg *= x;
	*colb *= x;

	*firea = 100;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD | NO_DECO;
	return 0;
}

void C5_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_C5";
	elem->Name = "C-5";
	elem->Colour = COLPACK(0x2050E0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
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

	elem->HeatConduct = 88;
	elem->Latent = 0;
	elem->Description = "Cold explosive, set off by anything cold.";

	elem->Properties = TYPE_SOLID | PROP_PHOTPASS | PROP_NEUTPENETRATE | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &C5_update;
	elem->Graphics = &C5_graphics;
	elem->Init = &C5_init_element;
}
