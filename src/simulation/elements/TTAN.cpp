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

int TTAN_update(UPDATE_FUNC_ARGS)
{
	int ttan = 0;
	if (nt <= 2)
		ttan = 2;
	else if (parts[i].tmp)
		ttan = 2;
	else if (nt <= 6)
		for (int nx=-1; nx <= 1; nx++)
		{
			for (int ny = -1; ny <= 1; ny++)
			{
				if ((!nx != !ny) && x+nx >= 0 && y+ny >= 0 && x+nx < XRES && y+ny < YRES)
				{
					if (TYP(pmap[y+ny][x+nx]) == PT_TTAN)
						ttan++;
				}
			}
		}
		
	if (ttan >= 2)
	{
		sim->air->blockair[y/CELL][x/CELL] = 1;
		sim->air->blockairh[y/CELL][x/CELL] = 0x8;
	}
	return 0;
}

void TTAN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_TTAN";
	elem->Name = "TTAN";
	elem->Colour = COLPACK(0x909090);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
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
	elem->Hardness = 48;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Titanium. Higher melting temperature than most other metals, blocks all air pressure.";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_HOT_GLOW|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1941.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &TTAN_update;
	elem->Graphics = NULL;
	elem->Init = &TTAN_init_element;
}
