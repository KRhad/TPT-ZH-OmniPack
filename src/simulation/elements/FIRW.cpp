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
#include "hmap.h"

int FIRW_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp <= 0)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					int rt = TYP(r);
					if (rt==PT_FIRE || rt==PT_PLSM || rt==PT_THDR)
					{
						float gx, gy;
						sim->GetGravityField(x, y, sim->elements[PT_FIRW].Gravity, 1.0f, gx, gy);
						if (gx*gx+gy*gy < 0.001f)
						{
							float angle = RNG::Ref().between(0, 6283) * 0.001f;//(in radians, between 0 and 2*pi)
							gx += sinf(angle) * sim->elements[PT_FIRW].Gravity * 0.5f;
							gy += cosf(angle) * sim->elements[PT_FIRW].Gravity * 0.5f;
						}
						parts[i].tmp = 1;
						parts[i].life = RNG::Ref().between(20, 29);
						float multiplier = (parts[i].life+20)*0.2f/sqrtf(gx*gx+gy*gy);
						parts[i].vx -= gx*multiplier;
						parts[i].vy -= gy*multiplier;
						return 0;
					}
				}
	}
	else if (parts[i].tmp == 1)
	{
		if (parts[i].life <= 0)
		{
			parts[i].tmp = 2;
		}
		else
		{
			parts[i].flags &= ~FLAG_STAGNANT;
		}
	}
	else //if (parts[i].tmp >= 2)
	{
		int caddress = RNG::Ref().between(0, 199) * 3;
		unsigned col = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
		for (int n = 0; n < 40; n++)
		{
			int np = sim->part_create(-3, x, y, PT_EMBR);
			if (np>-1)
			{
				float magnitude = RNG::Ref().between(40, 99) * 0.05f;
				float angle = RNG::Ref().between(0, 6283)* 0.001f; //(in radians, between 0 and 2*pi)
				parts[np].vx = parts[i].vx*0.5f + cosf(angle)*magnitude;
				parts[np].vy = parts[i].vy*0.5f + sinf(angle)*magnitude;
				parts[np].ctype = col;
				parts[np].tmp = 1;
				parts[np].life = RNG::Ref().between(70, 109);
				parts[np].temp = RNG::Ref().between(5750, 6249);
				parts[np].dcolour = parts[i].dcolour;
			}
		}
		sim->air->pv[y/CELL][x/CELL] += 8.0f;
		sim->part_kill(i);
		return 1;
	}
	return 0;
}

int FIRW_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->tmp > 0)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void FIRW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FIRW";
	elem->Name = "FIRW";
	elem->Colour = COLPACK(0xFFA040);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.2f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 55;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Fireworks! Colorful, set off by fire.";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FIRW_update;
	elem->Graphics = &FIRW_graphics;
	elem->Init = &FIRW_init_element;
}
