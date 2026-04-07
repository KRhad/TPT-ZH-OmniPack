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

int VIBR_update(UPDATE_FUNC_ARGS)
{
	int rndstore = 0;
	if (!parts[i].life) //if not exploding
	{
		//Heat absorption code
		if (parts[i].temp > 274.65f)
		{
			parts[i].tmp++;
			parts[i].temp -= 3;
		}
		else if (parts[i].temp < 271.65f)
		{
			parts[i].tmp--;
			parts[i].temp += 3;
		}
		//Pressure absorption code
		if (sim->air->pv[y/CELL][x/CELL] > 2.5)
		{
			parts[i].tmp += 7;
			sim->air->pv[y/CELL][x/CELL]--;
		}
		else if (sim->air->pv[y/CELL][x/CELL] < -2.5)
		{
			parts[i].tmp -= 2;
			sim->air->pv[y/CELL][x/CELL]++;
		}
		//initiate explosion counter
		if (parts[i].tmp > 1000)
			parts[i].life = 750;
	}
	else
	{
		//Release sparks before explode
		if (parts[i].life < 500)
			rndstore = RNG::Ref().gen();
		if (parts[i].life < 300)
		{
			int rx = rndstore%3-1;
			int ry = (rndstore>>2)%3-1;
			rndstore = rndstore >> 4;
			int r = pmap[y+ry][x+rx];
			if (TYP(r) && TYP(r) != PT_BREL && (sim->elements[TYP(r)].Properties&PROP_CONDUCTS) && !parts[ID(r)].life)
			{
				sim->spark_conductive(ID(r), x+rx, y+ry);
			}
		}
		//Release all heat
		if (parts[i].life < 500)
		{
			int rx = rndstore%7-3;
			int ry = (rndstore>>3)%7-3;
			int r = pmap[y+ry][x+rx];
			if (TYP(r) && TYP(r) != PT_VIBR && TYP(r) != PT_BVBR && !sim->IsHeatInsulator(sim->parts[ID(r)]))
			{
				parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp + parts[i].tmp * 3, MIN_TEMP, MAX_TEMP);
				parts[i].tmp = 0;
			}
		}
		//Explosion code
		if (parts[i].life == 1)
		{
			if (!parts[i].tmp2)
			{
				rndstore = RNG::Ref().gen();
				int index = sim->part_create(-3,x+((rndstore>>4)&3)-1,y+((rndstore>>6)&3)-1,PT_ELEC);
				if (index != -1)
					parts[index].temp = 7000;
				index = sim->part_create(-3,x+((rndstore>>8)&3)-1,y+((rndstore>>10)&3)-1,PT_PHOT);
				if (index != -1)
					parts[index].temp = 7000;
				int rx = ((rndstore>>12)&3)-1;
				rndstore = RNG::Ref().gen();
				index = sim->part_create(-1,x+rx-1,y+rndstore%3-1,PT_BREL);
				if (index != -1)
					parts[index].temp = 7000;
				sim->part_create(i, x, y, PT_EXOT);
				parts[i].tmp2 = (rndstore>>2)%1000;
				parts[i].temp=9000;
				sim->air->pv[y/CELL][x/CELL] += 50;

				return 1;
			}
			else
			{
				parts[i].tmp2 = 0;
				parts[i].temp = 273.15f;
				parts[i].tmp = 0;
			}
		}
	}
	//Neighbor check loop
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (parts[i].life)
				{
					//Makes VIBR/BVBR around it get tmp to start exploding too
					if ((TYP(r)==PT_VIBR  || TYP(r)==PT_BVBR))
					{
						if (!parts[ID(r)].life)
							parts[ID(r)].tmp += 45;
						else if (parts[i].tmp2 && parts[i].life > 75 && RNG::Ref().chance(1, 2))
						{
							parts[ID(r)].tmp2 = 1;
							parts[i].tmp = 0;
						}
					}
					//CFLM defuses it
					else if (TYP(r)==PT_HFLM)
					{
						parts[i].tmp2 = 1;
						parts[i].tmp = 0;
					}
				}
				else
				{
					//Melts into EXOT
					if (TYP(r) == PT_EXOT && RNG::Ref().chance(1, 25))
					{
						sim->part_create(i, x, y, PT_EXOT);
						return 1;
					}
				}
				//VIBR+ANAR=BVBR
				if (parts[i].type != PT_BVBR && TYP(r) == PT_ANAR)
				{
					part_change_type(i,x,y,PT_BVBR);
					sim->air->pv[y/CELL][x/CELL] -= 1;
				}
			}
	for (int trade = 0; trade < 9; trade++)
	{
		if (!(trade%2))
			rndstore = RNG::Ref().gen();
		int rx = rndstore%7-3;
		rndstore >>= 3;
		int ry = rndstore%7-3;
		rndstore >>= 3;
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (TYP(r) != PT_VIBR && TYP(r) != PT_BVBR)
				continue;
			if (parts[i].tmp > parts[ID(r)].tmp)
			{
				int transfer = parts[i].tmp - parts[ID(r)].tmp;
				parts[ID(r)].tmp += transfer/2;
				parts[i].tmp -= transfer/2;
				break;
			}
		}
	}
	parts[i].tmp = std::clamp(parts[i].tmp, 0, 1 << 15);
	return 0;
}

int VIBR_graphics(GRAPHICS_FUNC_ARGS)
{
	int gradient = cpart->tmp/10;
	if (gradient >= 100 || cpart->life)
	{
		*colr = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
		if (cpart->tmp2)
		{
			*colg = *colr;
			*colb = 255;
		}
		else
		{
			*colg = 255;
			*colb = *colr;
		}
		*firea = 90;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode = PMODE_NONE;
		*pixel_mode |= FIRE_BLEND;
	}
	else if (gradient < 100)
	{
		*colr += (int)restrict_flt(gradient*2.0f,0,255);
		*colg += (int)restrict_flt(gradient*2.0f,0,175);
		*colb += (int)restrict_flt(gradient*2.0f,0,255);
		*firea = (int)restrict_flt(gradient*.6f,0,60);
		*firer = *colr/2;
		*fireg = *colg/2;
		*fireb = *colb/2;
		*pixel_mode |= FIRE_BLEND;
	}
	return 0;
}

void VIBR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VIBR";
	elem->Name = "VIBR";
	elem->Colour = COLPACK(0x005000);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.85f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Vibranium. Stores energy and releases it in violent explosions.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &VIBR_update;
	elem->Graphics = &VIBR_graphics;
	elem->Init = &VIBR_init_element;
}
