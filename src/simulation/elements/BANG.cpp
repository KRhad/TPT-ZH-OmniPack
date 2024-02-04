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
#include "simulation/StructProperty.h"

int BANG_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp == 0)
	{
		if (parts[i].temp >= 673.0f)
			parts[i].tmp = 1;
		else
			for (int rx = -1; rx <= 1; rx++)
				for (int ry = -1; ry <= 1; ry++)
					if (rx || ry)
					{
						int r = pmap[y+ry][x+rx];
						if (!r)
							continue;
						if (TYP(r)==PT_FIRE || TYP(r)==PT_PLSM || TYP(r)==PT_SPRK || TYP(r)==PT_LIGH)
						{
							parts[i].tmp = 1;
						}
					}
	
	}
	else if (parts[i].tmp == 1)
	{
		if (pmap[y][x] && ID(pmap[y][x]) == (unsigned)i)
		{
			sim->FloodProp(x, y, particle::PropertyByName("tmp"), { 2 });
		}
		parts[i].tmp = 2;
	}
	else if (parts[i].tmp == 2)
	{
		parts[i].tmp = 3;
	}
	else
	{
		float otemp = parts[i].temp-273.15f;
		//Explode!!
		sim->air->pv[y/CELL][x/CELL] += 0.5f;
		parts[i].tmp = 0;
		if (RNG::Ref().chance(1, 3))
		{
			if (RNG::Ref().chance(1, 2))
			{
				sim->part_create(i, x, y, PT_FIRE);
			}
			else
			{
				sim->part_create(i, x, y, PT_SMKE);
				parts[i].life = RNG::Ref().between(500, 549);
			}
			parts[i].temp = restrict_flt((MAX_TEMP/4)+otemp, MIN_TEMP, MAX_TEMP);
		}
		else
		{
			if (RNG::Ref().chance(1, 15))
			{
				sim->part_create(i, x, y, PT_EMBR);
				parts[i].temp = restrict_flt((MAX_TEMP/3)+otemp, MIN_TEMP, MAX_TEMP);
				parts[i].vx = RNG::Ref().between(-10, 10);
				parts[i].vy = RNG::Ref().between(-10, 10);
			}
			else
			{
				sim->part_kill(i);
			}
		}
		return 1;
	}
	return 0;
}

void BANG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BANG";
	elem->Name = "TNT";
	elem->Colour = COLPACK(0xC05050);
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
	elem->Description = "TNT, explodes all at once.";

	elem->Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BANG_update;
	elem->Graphics = NULL;
	elem->Init = &BANG_init_element;
}
