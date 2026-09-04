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

int INVS_update(UPDATE_FUNC_ARGS)
{
	float pressureResistance = 0.0f;
	if (parts[i].tmp > 0)
		pressureResistance = (float) parts[i].tmp;
	else
		pressureResistance = 4.0f;

	if (sim->air->pv[y/CELL][x/CELL] < -pressureResistance || sim->air->pv[y/CELL][x/CELL] > pressureResistance)
		parts[i].tmp2 = 1;
	else
		parts[i].tmp2 = 0;
	return 0;
}

int INVS_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp2)
	{
		*cola = 100;
		*colr = 15;
		*colg = 0;
		*colb = 150;
		*pixel_mode = PMODE_BLEND;
	} 
	return 0;
}

void INVIS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_INVIS";
	elem->Name = "INVS";
	elem->Colour = COLPACK(0x00CCCC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SENSOR;
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
	elem->Hardness = 15;

	elem->Weight = 100;

	elem->HeatConduct = 164;
	elem->Latent = 0;
	elem->Description = "在压力下不可见，允许颗粒通过。";
	elem->DetailedDescription = "描述：当施加压力时对粒子隐形，使物质通过。在不施加压力时，光子(PHOT)可以通过它并变成中子(NEUT)，在4 P 左右时隐形。\n元素参数：Tmp=1 时隐形；Tmp=0 时还原\n导热率：164\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_PHOTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &INVS_update;
	elem->Graphics = &INVS_graphics;
	elem->Init = &INVIS_init_element;
}
