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

int FUSE_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life <= 0)
	{
		int r = sim->part_create(i, x, y, PT_PLSM);
		if (r > -1)
			parts[r].life = 50;
		return 1;
	}
	else if (parts[i].life < 40)
	{
		parts[i].life--;
		if (RNG::Ref().chance(1, 100))
		{
			int r = sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), PT_PLSM);
			if (r > -1)
				parts[r].life = 50;
		}
	}

	if (sim->air->pv[y/CELL][x/CELL] > 2.7f && parts[i].tmp>40)
		parts[i].tmp=39;
	else if (parts[i].tmp <= 0)
	{
		sim->part_create(i, x, y, PT_FSEP);
		return 1;
	}
	else if (parts[i].tmp < 40)
		parts[i].tmp--;

	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_SPRK || (parts[i].temp>=(273.15+700.0f) && RNG::Ref().chance(1, 20)))
				{
					if (parts[i].life > 40)
						parts[i].life = 39;
				}
			}
	return 0;
}

void FUSE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FUSE";
	elem->Name = "FUSE";
	elem->Colour = COLPACK(0x0A5706);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->HotAir = 0.0f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 19;

	elem->Weight = 100;

	elem->HeatConduct = 200;
	elem->Latent = 0;
	elem->Description = "缓燃。高温或火花点燃。";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 50;
	elem->DefaultProperties.tmp = 50;

	elem->Update = &FUSE_update;
	elem->Graphics = NULL;
	elem->Init = &FUSE_init_element;
}
