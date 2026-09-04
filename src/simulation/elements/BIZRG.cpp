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

int BIZR_update(UPDATE_FUNC_ARGS);
int BIZR_graphics(GRAPHICS_FUNC_ARGS);

void BIZRG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BIZRG";
	elem->Name = "BIZG";
	elem->Colour = COLPACK(0x00FFBB);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 2.75f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP - 200.0f + 273.15f;
	elem->HeatConduct = 42;
	elem->Latent = 0;
	elem->Description = "奇异气体。";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 100.0f;
	elem->HighTemperatureTransitionElement = PT_BIZR;

	elem->DefaultProperties.ctype = 0x47FFFF;

	elem->Update = &BIZR_update;
	elem->Graphics = &BIZR_graphics;
	elem->Init = &BIZRG_init_element;
}
