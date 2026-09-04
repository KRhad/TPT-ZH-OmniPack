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

int CLST_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_WATR)
				{
					if (RNG::Ref().chance(1, 1500))
					{
						sim->part_create(i, x, y, PT_PSTS);
						sim->part_kill(ID(r));
					}
				}
				else if (TYP(r) == PT_NITR)
				{
					sim->part_create(i, x, y, PT_BANG);
					sim->part_create(ID(r), x+rx, y+ry, PT_BANG);
				}
				else if (TYP(r) == PT_CLST)
				{
					float cxy = 0;
					if (parts[i].temp < 195)
						cxy = 0.05f;
					else if (parts[i].temp < 295)
						cxy = 0.015f;
					else if (parts[i].temp < 350)
						cxy = 0.01f;
					else
						cxy = 0.005f;
					parts[i].vx += cxy*rx;
					parts[i].vy += cxy*ry;//These two can be set not to calculate over 350 later. They do virtually nothing over 0.005.
				}
			}
	return 0;
}

int CLST_graphics(GRAPHICS_FUNC_ARGS)
{
	// speckles!
	int z = (cpart->tmp - 5) * 16;
	*colr += z;
	*colg += z;
	*colb += z;
	return 0;
}

void CLST_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = RNG::Ref().between(0, 6);
}

void CLST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CLST";
	elem->Name = "CLST";
	elem->Colour = COLPACK(0xE4A4A4);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.2f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 2;
	elem->Hardness = 2;

	elem->Weight = 55;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "粘土尘。与水混合成浆。";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1256.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &CLST_update;
	elem->Graphics = &CLST_graphics;
	elem->Func_Create = &CLST_create;
	elem->Init = &CLST_init_element;
}
