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

int ISZ_update(UPDATE_FUNC_ARGS);

void ISOZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ISOZ";
	elem->Name = "ISOZ";
	elem->Colour = COLPACK(0xAA30D0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 24;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "同位素Z。放射性液体。触光子或负压衰变。";

	elem->Properties = TYPE_LIQUID | PROP_NEUTPENETRATE | PROP_PHOTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 160.0f;
	elem->LowTemperatureTransitionElement = PT_ISZS;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &ISZ_update;
	elem->Graphics = NULL;
	elem->Init = &ISOZ_init_element;
}
