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

int FSEP_update(UPDATE_FUNC_ARGS)
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
		if (RNG::Ref().chance(1, 10))
		{
			int r = sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), PT_PLSM);
			if (r > -1)
				parts[r].life = 50;
		}
	}
	else
	{
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_SPRK || (parts[i].temp>=(273.15+400.0f))) && RNG::Ref().chance(1, 15))
					{
						parts[i].life = 39;
						return 0;
					}
				}
	}
	return 0;
}

void FSEP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FSEP";
	elem->Name = "FSEP";
	elem->Colour = COLPACK(0x63AD5F);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 27;

	elem->Weight = 70;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Fuse Powder. Burns slowly like FUSE.";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 50;

	elem->Update = &FSEP_update;
	elem->Graphics = NULL;
	elem->Init = &FSEP_init_element;
}
