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

void BIZRS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BIZRS";
	elem->Name = "BIZS";
	elem->Colour = COLPACK(0x00E455);
	elem->MenuVisible = 0;
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
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP + 300.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "奇异固体。";
	elem->DetailedDescription = "描述：与一般物理规律相反的液体，高温时凝固，低温时汽化，用颜色工具改变它的颜色后，它将把其他与之相遇的物质染成它的颜色。同时，它还能将光子(PHOT)转换成电子(ELEC)。\n沸点：-173.15℃/100K\n凝固点：126.85℃/400K\n导热率：29/42/251\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 400.0f;
	elem->LowTemperatureTransitionElement = PT_BIZR;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.ctype = 0x47FFFF;

	elem->Update = &BIZR_update;
	elem->Graphics = &BIZR_graphics;
	elem->Init = &BIZRS_init_element;
}
