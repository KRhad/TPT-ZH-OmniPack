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

int ROCK_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = (cpart->tmp2 - 7) * 6; // Randomized color noise based on tmp2
	*colr += z;
	*colg += z;
	*colb += z;
	
	if (cpart->temp >= 810.15) // Glows when hot, right before melting becomes bright
	{
		*pixel_mode |= FIRE_ADD;
		
		*firea = ((cpart->temp)-810.15)/45;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
	}
	return 0;
}

void ROCK_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp2 = RNG::Ref().between(0, 10);
}

void ROCK_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ROCK";
	elem->Name = "ROCK";
	elem->Colour = PIXPACK(0x727272);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 5;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 200;
	elem->Description = "Solid, melts into various elements.";

	elem->Properties = TYPE_SOLID | PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 120;
	elem->HighPressureTransitionElement = PT_STNE;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1943.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Graphics = &ROCK_graphics;
	elem->Func_Create = &ROCK_create;
}
