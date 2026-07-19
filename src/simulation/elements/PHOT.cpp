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
#include "simulation/elements/FILT.h"

int FIRE_update(UPDATE_FUNC_ARGS);

int PHOT_update(UPDATE_FUNC_ARGS)
{
	if (!(parts[i].ctype&0x3FFFFFFF))
	{
		sim->part_kill(i);
		return 1;
	}
	if (parts[i].temp > 506.0f)
		if (RNG::Ref().chance(1, 10))
			FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);

	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r)==PT_ISOZ || TYP(r)==PT_ISZS)
			{
				if (RNG::Ref().chance(1, 400))
				{
					parts[i].vx *= 0.90f;
					parts[i].vy *= 0.90f;
					sim->part_create(ID(r), x+rx, y+ry, PT_PHOT);
					float rrr = RNG::Ref().between(0, 359) * M_PI / 180.0f, rr;
					if (TYP(r) == PT_ISOZ)
						rr = RNG::Ref().between(128, 255) / 127.0f;
					else
						rr = RNG::Ref().between(128, 255) / 127.0f;
					parts[ID(r)].vx = rr*cosf(rrr);
					parts[ID(r)].vy = rr*sinf(rrr);
					sim->air->pv[y/CELL][x/CELL] -= 15.0f * CFDS;
				}
			}
			else if (TYP(r) == PT_QRTZ || TYP(r) == PT_PQRT)
			{
				if (!ry && !rx)
				{
					float a = RNG::Ref().between(0, 359) * M_PI / 180.0f;
					parts[i].vx = 3.0f*cosf(a);
					parts[i].vy = 3.0f*sinf(a);
					if (parts[i].ctype == 0x3FFFFFFF)
						parts[i].ctype = 0x1F << RNG::Ref().between(0, 25);
					if (parts[i].life)
						parts[i].life++; //Delay death
				}
			}
			else if (TYP(r) == PT_BGLA)
			{
				if (!ry && !rx)
				{
					float a = RNG::Ref().between(-50, 50) * 0.001f;
					float rx = cosf(a), ry = sinf(a), vx, vy;
					vx = rx * parts[i].vx + ry * parts[i].vy;
					vy = rx * parts[i].vy - ry * parts[i].vx;
					parts[i].vx = vx;
					parts[i].vy = vy;
				}
			}
			else if (TYP(r) == PT_RSST && !ry && !rx)//if on RSST, make it solid
			{
				int ct_under = parts[ID(r)].ctype;
				int tmp_under = parts[ID(r)].tmp;

				//If there's a correct ctype set, solidify RSST into it
				if (ct_under > 0 && ct_under < PT_NUM)
				{
					sim->part_create(ID(r), x, y, ct_under);

					//If there's a correct tmp set, use it for ctype
					if ((tmp_under > 0) && (tmp_under < PT_NUM) && (sim->elements[ct_under].CarriesTypeIn & (1U << FIELD_CTYPE)))
						parts[ID(r)].ctype = tmp_under;
				}
				else
					sim->part_change_type(ID(r), x, y, PT_RSSS); //Default to RSSS if no ctype

				sim->part_kill(i);

				return 1;
			}
			else if (TYP(r) == PT_FILT)
			{
				if (parts[ID(r)].tmp == 9)
				{
					parts[i].vx += RNG::Ref().between(-500, 500) / 1000.0f;
					parts[i].vy += RNG::Ref().between(-500, 500) / 1000.0f;
				}
			}
		}
	}

	return 0;
}

int PHOT_graphics(GRAPHICS_FUNC_ARGS)
{
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
	if (cpart->flags & FLAG_PHOTDECO)
	{
		*pixel_mode &= ~NO_DECO;
	}
	return 0;
}

void PHOT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = RNG::Ref().between(0, 7) * 0.78540f;
	sim->parts[i].vx = 3.0f * cosf(a);
	sim->parts[i].vy = 3.0f * sinf(a);
	if (TYP(pmap[y][x]) == PT_FILT)
		sim->parts[i].ctype = interactWavelengths(&sim->parts[ID(pmap[y][x])], sim->parts[i].ctype);
}

void PHOT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PHOT";
	elem->Name = "PHOT";
	elem->Colour = COLPACK(0xFFFFFF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP + 900.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Photons. Refract through glass, and different elements change its color. Can ignite flammable materials.";

	elem->Properties = TYPE_ENERGY | PROP_PHOTPASS | PROP_LIFE_DEC | PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 680;
	elem->DefaultProperties.ctype = 0x3FFFFFFF;

	elem->Update = &PHOT_update;
	elem->Graphics = &PHOT_graphics;
	elem->Func_Create = &PHOT_create;
	elem->Init = &PHOT_init_element;
}
