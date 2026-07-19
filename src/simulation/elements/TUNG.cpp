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

int TUNG_update(UPDATE_FUNC_ARGS)
{
	const float MELTING_POINT = sim->elements[PT_TUNG].HighTemperatureTransitionThreshold;
	bool splode = false;
	if(parts[i].temp > 2400.0)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (TYP(r) == PT_O2)
					{
						splode = true;
					}
				}
	}
	if ((parts[i].temp > MELTING_POINT && RNG::Ref().chance(1, 20)) || splode)
	{
		if (RNG::Ref().chance(1, 50))
		{
			sim->air->pv[y/CELL][x/CELL] += 50.0f;
		}
		else if (RNG::Ref().chance(1, 100))
		{
			part_change_type(i, x, y, PT_FIRE);
			parts[i].life = RNG::Ref().between(0, 499);
			return 1;
		}
		else
		{
			part_change_type(i, x, y, PT_LAVA);
			parts[i].ctype = PT_TUNG;
			return 1;
		}
		if(splode)
		{
			parts[i].temp = restrict_flt(MELTING_POINT + RNG::Ref().between(200, 799), MIN_TEMP, MAX_TEMP);
		}
		parts[i].vx += RNG::Ref().between(-50, 50);
		parts[i].vy += RNG::Ref().between(-50, 50);
		return 1;
	}
	int press = int(sim->air->pv[y/CELL][x/CELL] * 64);
	int diff = press - parts[i].tmp3;
	if (diff > 32 || diff < -32)
	{
		part_change_type(i,x,y,PT_BRMT);
		parts[i].ctype = PT_TUNG;
		return 1;
	}
	parts[i].tmp3 = press;
	return 0;
}

int TUNG_graphics(GRAPHICS_FUNC_ARGS)
{
	const float MELTING_POINT = sim->elements[PT_TUNG].HighTemperatureTransitionThreshold;
	double startTemp = (MELTING_POINT - 1500.0);
	double tempOver = (((cpart->temp - startTemp)/1500.0)*M_PI) - (M_PI/2.0);
	if(tempOver > -(M_PI/2.0))
	{
		double gradv;
		if(tempOver > (M_PI/2.0))
			tempOver = (M_PI/2.0);
		gradv = sin(tempOver) + 1.0;
		*firer = (int)(gradv * 258.0);
		*fireg = (int)(gradv * 156.0);
		*fireb = (int)(gradv * 112.0);
		*firea = 30;

		*colr += *firer;
		*colg += *fireg;
		*colb += *fireb;
		*pixel_mode |= FIRE_ADD;
	}
	return 0;
}

void TUNG_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp3 = int(sim->air->pv[y/CELL][x/CELL] * 64);
}

void TUNG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_TUNG";
	elem->Name = "TUNG";
	elem->Colour = COLPACK(0x505050);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
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
	elem->Meltable = 1;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Tungsten. Brittle metal with a very high melting point.";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 3695.0f; // TUNG melts in its update function instead of in the normal way, but store the threshold here so that it can be changed from Lua
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &TUNG_update;
	elem->Graphics = &TUNG_graphics;
	elem->Func_Create = &TUNG_create;
	elem->Init = &TUNG_init_element;
}
