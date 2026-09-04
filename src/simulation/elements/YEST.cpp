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

int YEST_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_DYST && RNG::Ref().chance(1, 6) && !legacy_enable)
				{
					part_change_type(i,x,y,PT_DYST);
				}
			}
	if (parts[i].temp>303 && parts[i].temp<317)
	{
		sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), PT_YEST);
	}
	return 0;
}

void YEST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_YEST";
	elem->Name = "YEST";
	elem->Colour = COLPACK(0xEEE0C0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
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

	elem->Flammable = 15;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 31;

	elem->Weight = 80;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "酵母。温暖时生长，约37°C。";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_DYST;

	elem->Update = &YEST_update;
	elem->Graphics = NULL;
	elem->Init = &YEST_init_element;
}
