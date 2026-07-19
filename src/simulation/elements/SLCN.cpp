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

static const int SLCN_COLOUR[16] = {
	PIXPACK(0x5A6679), PIXPACK(0x6878A1), PIXPACK(0xABBFDD), PIXPACK(0x838490),
	PIXPACK(0xBCCDDF), PIXPACK(0x82A0D2), PIXPACK(0x5B6680), PIXPACK(0x232C3B),
	PIXPACK(0x485067), PIXPACK(0x8B9AB6), PIXPACK(0xADB1C1), PIXPACK(0xC3C6D1),
	PIXPACK(0x8594AD), PIXPACK(0x262F47), PIXPACK(0xA9AEBC), PIXPACK(0xC2E1F7),
};

static void initSparkles(particle &part)
{
	// bits 31-20: phase increment (randomised to a value between 1 and 9)
	// bits 19-16: next colour index
	// bits 15-12: current colour index
	// bits 11-00: phase
	part.tmp = RNG::Ref().between(0x100000, 0x9FFFFF);
}

int SLCN_update(UPDATE_FUNC_ARGS)
{
	if (!parts[i].tmp)
	{
		initSparkles(parts[i]);
	}
	int phase = (parts[i].tmp & 0xFFF) + ((parts[i].tmp >> 20) & 0xFFF);
	if (phase & 0x1000)
	{
		// discard current, current <- next, next <- random, wrap phase
		parts[i].tmp = (parts[i].tmp & 0xFFF00000) | (phase & 0xFFF) | (RNG::Ref().between(0, 15) << 16) | ((parts[i].tmp >> 4) & 0xF000);
	}
	else
	{
		// update phase
		parts[i].tmp = (parts[i].tmp & 0xFFFFF000) | phase;
	}
	
	if (parts[i].life == 0 && parts[i].temp < 373.15f)
	{
		for (int j = 0; j < 4; j++)
		{
			static const int check_coords_x[] = { -4, 4, 0, 0 };
			static const int check_coords_y[] = { 0, 0, -4, 4 };
			int n = pmap[y + check_coords_y[j]][x + check_coords_x[j]];
			if (n && TYP(n) == PT_SPRK)
			{
				particle &neighbour = parts[ID(n)];
				if (neighbour.life != 0 && neighbour.life < 4)
				{
					sim->part_change_type(i, x, y, PT_SPRK);
					parts[i].life = 4;
					parts[i].ctype = PT_SLCN;
				}
			}
		}
	}
	
	return 0;
}

int SLCN_graphics(GRAPHICS_FUNC_ARGS)
{
	int curr_colour = SLCN_COLOUR[(cpart->tmp >> 12) & 15];
	if (cpart->tmp & 0x800) // mix with next colour if phase is at least halfway there
	{
		int next_colour = SLCN_COLOUR[(cpart->tmp >> 16) & 15];
		curr_colour = PIXRGB(
			(PIXR(curr_colour) + PIXR(next_colour)) / 2,
							 (PIXG(curr_colour) + PIXG(next_colour)) / 2,
							 (PIXB(curr_colour) + PIXB(next_colour)) / 2
		);
	}
	*colr = PIXR(curr_colour);
	*colg = PIXG(curr_colour);
	*colb = PIXB(curr_colour);
	
	int rnd = (cpart->tmp & 0xFFFF) * ((cpart->tmp >> 16) & 0xFFFF);
	if (!(rnd % 887))
	{
		*pixel_mode |= PMODE_FLARE | PMODE_GLOW;
	}
	if (!(rnd % 593))
	{
		*pixel_mode |= PMODE_SPARK;
	}
	
	return 0;
}

void SLCN_create(ELEMENT_CREATE_FUNC_ARGS)
{
	initSparkles(sim->parts[i]);
}

void SLCN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SLCN";
	elem->Name = "SLCN";
	elem->Colour = PIXPACK(0xBCCDDF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.27f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 90;

	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->Description = "Powdered Silicon. A key ingredient in producing multiple materials.";

	elem->Properties = TYPE_PART | PROP_CONDUCTS | PROP_HOT_GLOW | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 3538.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &SLCN_update;
	elem->Graphics = &SLCN_graphics;
	elem->Func_Create = &SLCN_create;
	elem->Init = &SLCN_init_element;
}
