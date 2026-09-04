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

void LNTG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LNTG";
	elem->Name = "LN2";
	elem->Colour = COLPACK(0x80A0DF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
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

	elem->Weight = 30;

	elem->DefaultProperties.temp = 70.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "液氮。非常冷，一接触到温暖的东西就会消失。";
	elem->DetailedDescription = "描述：液氮，遇到比它热的物质后会消失并产生压力。LN2 和 NICE(氮冰)通常用作冷却机制，因为LN2 在任何温度下都会消失。\n沸点：-196.15℃/77.0K(消失)\n凝固点：-210.15℃/63K\n导热率：70\n初始温度：-205.00℃/68.15K";

	elem->Properties = TYPE_LIQUID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 63.0f;
	elem->LowTemperatureTransitionElement = PT_NICE;
	elem->HighTemperatureTransitionThreshold = 77.0f;
	elem->HighTemperatureTransitionElement = PT_NONE;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &LNTG_init_element;
}
