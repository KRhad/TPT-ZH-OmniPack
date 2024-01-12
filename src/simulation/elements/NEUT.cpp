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

void DeutExplosion(Simulation *sim, int n, int x, int y, float temp, int t)
{
	int i;
	n = (n/50);
	if (n < 1)
		n = 1;
	else if (n > 340)
		n = 340;

	for (int c = 0; c < n; c++)
	{
		i = sim->part_create(-3, x, y, t);
		if (i >= 0)
			parts[i].temp = temp;
		else if (sim->pfree < 0)
			break;
	}
	sim->air->pv[y/CELL][x/CELL] += (6.0f * CFDS)*n;
	return;
}

int FIRE_update(UPDATE_FUNC_ARGS);

int NEUT_update(UPDATE_FUNC_ARGS)
{
	int pressureFactor = 3 + (int)sim->air->pv[y/CELL][x/CELL];
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				switch (TYP(r))
				{
				case PT_WATR:
					if (RNG::Ref().chance(3, 20))
						part_change_type(ID(r), x+rx, y+ry, PT_DSTW);
					//no break
				case PT_ICEI:
				case PT_SNOW:
					parts[i].vx *= 0.995f;
					parts[i].vy *= 0.995f;
					break;
				case PT_PLUT:
					if (RNG::Ref().chance(pressureFactor, 1000))
					{
						if (RNG::Ref().chance(1, 3))
						{
							sim->part_create(ID(r), x+rx, y+ry, RNG::Ref().chance(2, 3) ? PT_LAVA : PT_URAN);
							parts[ID(r)].temp = MAX_TEMP;
							if (parts[ID(r)].type == PT_LAVA)
							{
								parts[ID(r)].tmp = 100;
								parts[ID(r)].ctype = PT_PLUT;
							}
						}
						else
						{
							sim->part_create(ID(r), x+rx, y+ry, PT_NEUT);
							parts[ID(r)].vx = 0.25f*parts[ID(r)].vx + parts[i].vx;
							parts[ID(r)].vy = 0.25f*parts[ID(r)].vy + parts[i].vy;
						}
						sim->air->pv[y/CELL][x/CELL] += 10.0f * CFDS; //Used to be 2, some people said nukes weren't powerful enough
						FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);
					}
					break;
#ifdef SDEUT
				case PT_DEUT:
					if (RNG::Ref().chance(pressureFactor + 1 + (parts[ID(r)].life/100), 1000))
					{
						DeutExplosion(sim, parts[ID(r)].life, x+rx, y+ry, restrict_flt(parts[ID(r)].temp + parts[ID(r)].life*500.0f, MIN_TEMP, MAX_TEMP), PT_NEUT);
						sim->part_kill(ID(r));
					}
					break;
#else
				case PT_DEUT:
					if (RNG::Ref().chance(pressureFactor + 1, 1000))
					{
						sim->part_create(ID(r), x+rx, y+ry, PT_NEUT);
						parts[ID(r)].vx = 0.25f*parts[ID(r)].vx + parts[i].vx;
						parts[ID(r)].vy = 0.25f*parts[ID(r)].vy + parts[i].vy;
						if (parts[ID(r)].life>0)
						{
							parts[ID(r)].life --;
							parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp + parts[ID(r)].life*17.0f, MIN_TEMP, MAX_TEMP);
							pv[y/CELL][x/CELL] += 6.0f * CFDS;
						}
						else
							sim->part_kill(ID(r));
					}
					break;
#endif
				case PT_GUNP:
					if (RNG::Ref().chance(3, 200))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_DUST);
					break;
				case PT_DYST:
					if (RNG::Ref().chance(3, 200))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_YEST);
					break;
				case PT_YEST:
					sim->part_change_type(ID(r), x+rx, y+ry, PT_DYST);
					break;
				case PT_PLEX:
					if (RNG::Ref().chance(3, 200))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_GOO);
					break;
				case PT_NITR:
					if (RNG::Ref().chance(3, 200))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_DESL);
					break;
				case PT_PLNT:
					if (RNG::Ref().chance(1, 20))
						sim->part_create(ID(r), x+rx, y+ry, PT_WOOD);
					break;
				case PT_DESL:
				case PT_OIL:
					if (RNG::Ref().chance(3, 200))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_GAS);
					break;
				case PT_COAL:
					if (RNG::Ref().chance(1, 20))
						sim->part_create(ID(r), x+rx, y+ry, PT_WOOD);
					break;
				case PT_BCOL:
					if (RNG::Ref().chance(1, 20))
						sim->part_create(ID(r), x+rx, y+ry, PT_SAWD);
					break;
				case PT_DUST:
					if (RNG::Ref().chance(1, 20))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_FWRK);
					break;
				case PT_EMBR:
					if (parts[i].tmp == 1 && RNG::Ref().chance(1, 20))
						sim->part_change_type(ID(r), x+rx, y+ry, PT_FWRK);
					break;
				case PT_FWRK:
					if (RNG::Ref().chance(1, 20))
						parts[ID(r)].ctype = PT_DUST;
					break;
				case PT_ACID:
					if (RNG::Ref().chance(1, 20))
						sim->part_create(ID(r), x+rx, y+ry, PT_ISOZ);
					break;
				case PT_TTAN:
					if (RNG::Ref().chance(1, 20))
					{
						sim->part_kill(i);
						return 1;
					}
					break;
				case PT_EXOT:
					if (RNG::Ref().chance(1, 20))
						parts[ID(r)].life = 1500;
					break;
				case PT_RFRG:
					if (RNG::Ref().chance(1, 2))
						sim->part_create(ID(r), x+rx, y+ry, PT_GAS);
					else
						sim->part_create(ID(r), x+rx, y+ry, PT_CAUS);
					break;
				default:
					break;
				}
			}
	return 0;
}

int NEUT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 120;
	*firer = 10;
	*fireg = 80;
	*fireb = 120;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD;
	return 1;
}

void NEUT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float r = RNG::Ref().between(128, 255) / 127.0f;
	float a = RNG::Ref().between(0, 359) * 3.14159f / 180.0f;
	sim->parts[i].life = RNG::Ref().between(480, 959);
	sim->parts[i].vx = r * cosf(a);
	sim->parts[i].vy = r * sinf(a);
}

void NEUT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_NEUT";
	elem->Name = "NEUT";
	elem->Colour = COLPACK(0x20E0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.01f;
	elem->HotAir = 0.002f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP + 4.0f + 273.15f;
	elem->HeatConduct = 60;
	elem->Latent = 0;
	elem->Description = "Neutrons. Interact with matter in odd ways.";

	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &NEUT_update;
	elem->Graphics = &NEUT_graphics;
	elem->Func_Create = &NEUT_create;
	elem->Init = &NEUT_init_element;
}
