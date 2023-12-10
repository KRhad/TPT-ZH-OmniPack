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

int VIRS_update(UPDATE_FUNC_ARGS);

int VRSG_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode &= ~PMODE;
	*pixel_mode |= FIRE_BLEND;
	*firer = *colr/2;
	*fireg = *colg/2;
	*fireb = *colb/2;
	*firea = 125;
	*pixel_mode |= NO_DECO;
	return 1;
}

void VRSG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VRSG";
	elem->Name = "VRSG";
	elem->Colour = COLPACK(0xFE68FE);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.75f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 500;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = 522.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Gas Virus. Turns everything it touches into virus.";

	elem->Properties = TYPE_GAS;
	elem->CarriesTypeIn = 1U << FIELD_TMP2;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 673.0f;
	elem->LowTemperatureTransitionElement = PT_VIRS;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp4 = 250;

	elem->Update = &VIRS_update;
	elem->Graphics = &VRSG_graphics;
	elem->Init = &VRSG_init_element;
}
