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

int NBLE_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 5273.15 && sim->air->pv[y/CELL][x/CELL] > 100.0f)
	{
		parts[i].tmp |= 0x1;
		if (RNG::Ref().chance(1, 5))
		{
			int j;
			float temp = parts[i].temp;
			sim->part_create(i,x,y,PT_CO2);

			j = sim->part_create(-3,x,y,PT_NEUT);
			if (j != -1)
				parts[j].temp = temp;
			if (RNG::Ref().chance(1, 25))
			{
				j = sim->part_create(-3,x,y,PT_ELEC);
				if (j != -1)
					parts[j].temp = temp;
			}
			j = sim->part_create(-3,x,y,PT_PHOT);
			if (j != -1)
			{
				parts[j].ctype = 0xF800000;
				parts[j].temp = temp;
				parts[j].tmp = 0x1;
			}

			int rx = x + RNG::Ref().between(-1, 1), ry = y + RNG::Ref().between(-1, 1), rt = TYP(pmap[ry][rx]);
			if (sim->can_move[PT_PLSM][rt] || rt == PT_NBLE)
			{
				j = sim->part_create(-3,rx,ry,PT_PLSM);
				if (j > -1)
				{
					parts[j].temp = temp;
					parts[j].tmp |= 4;
				}
			}

			parts[i].temp = temp + RNG::Ref().between(1750, 2249);
			sim->air->pv[y/CELL][x/CELL] += 50;
		}
	}
	return 0;
}

void NBLE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_NBLE";
	elem->Name = "NBLE";
	elem->Colour = COLPACK(0xEB4917);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.75f;
	elem->HotAir = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x3FFF8000;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP + 2.0f + 273.15f;
	elem->HeatConduct = 106;
	elem->Latent = 0;
	elem->Description = "Noble Gas. Ionizes into plasma when sparked. Diffuses.";

	elem->Properties = TYPE_GAS|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &NBLE_update;
	elem->Graphics = NULL;
	elem->Init = &NBLE_init_element;
}
