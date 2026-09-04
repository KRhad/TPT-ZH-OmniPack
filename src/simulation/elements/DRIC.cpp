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

void DRIC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DRIC";
	elem->Name = "DRIC";
	elem->Colour = COLPACK(0xE0E0E0);
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

	elem->DefaultProperties.temp = 172.65f;
	elem->HeatConduct = 2;
	elem->Latent = 0;
	elem->Description = "干冰，CO2冷却时形成。";
	elem->DetailedDescription = "描述：干冰，当二氧化碳(CO2)温度为-78.5℃以下时形成。当 DRIC 被加热到-80℃以上时，即使在高温下，也需要一段时间才能重新变成 CO2。\n升华点：-77.5℃/195.65K\n导热率：2\n初始温度：-100.50℃/172.65K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 195.65f;
	elem->HighTemperatureTransitionElement = PT_CO2;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &DRIC_init_element;
}
