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

int MORT_update(UPDATE_FUNC_ARGS)
{
	sim->part_create(-1, x, y-1, PT_SMKE);
	return 0;
}

void MORT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_MORT";
	elem->Name = "MORT";
	elem->Colour = COLPACK(0xE0E0E0);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.01f;
	elem->HotAir = 0.002f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP + 4.0f + 273.15f;
	elem->HeatConduct = 60;
	elem->Latent = 0;
	elem->Description = "蒸汽火车。";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	// CHOO CHOO
	elem->DefaultProperties.vx = 2.0f;

	elem->Update = &MORT_update;
	elem->Graphics = NULL;
	elem->Init = &MORT_init_element;
}
