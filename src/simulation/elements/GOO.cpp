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

#define ADVECTION 0.1f

int GOO_update(UPDATE_FUNC_ARGS)
{
	if (!parts[i].life && sim->air->pv[y/CELL][x/CELL] > 1.0f)
		parts[i].life = RNG::Ref().between(300, 379);
	if (parts[i].life)
	{
		parts[i].vx += ADVECTION * sim->air->vx[y/CELL][x/CELL];
		parts[i].vy += ADVECTION * sim->air->vy[y/CELL][x/CELL];
	}
	return 0;
}

void GOO_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GOO";
	elem->Name = "GOO";
	elem->Colour = COLPACK(0x804000);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.50f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 12;
	elem->PhotonReflectWavelengths = 0x3FFAAA00;

	elem->Weight = 100;

	elem->HeatConduct = 75;
	elem->Latent = 0;
	elem->Description = "Deforms and disappears under pressure.";

	elem->Properties = TYPE_SOLID | PROP_NEUTPENETRATE|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &GOO_update;
	elem->Graphics = NULL;
	elem->Init = &GOO_init_element;
}
