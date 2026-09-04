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

void NICE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_NICE";
	elem->Name = "NICE";
	elem->Colour = COLPACK(0xC0E0FF);
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
	elem->HotAir = -0.0005f* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 35.0f;
	elem->HeatConduct = 46;
	elem->Latent = 0;
	elem->Description = "氮冰。非常冷，稍微加热就会融化成LN2。";
	elem->DetailedDescription = "描述：氮的固体形式。熔化后变成 LN2，当过度加热时，LN2 又会消失。对于 NICE 来说大多数材料的温度都足够高，可以将其熔化成液体形式，并且大概率使液体沸腾。\n熔点：-210.05℃/63.1K\n导热率：46\n初始温度：-238.15℃/35K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 63.1f;
	elem->HighTemperatureTransitionElement = PT_LNTG;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &NICE_init_element;
}
