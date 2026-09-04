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

int NPTCT_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 295.0f)
		parts[i].temp -= 2.5f;
	return 0;
}

void PTCT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PTCT";
	elem->Name = "PTCT";
	elem->Colour = COLPACK(0x405050);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "PTC热敏电阻，低于100℃时导通PSCN与NSCN。";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1687.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &NPTCT_update;
	elem->Graphics = NULL;
	elem->Init = &PTCT_init_element;
}
