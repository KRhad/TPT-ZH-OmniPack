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

void RBDM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RBDM";
	elem->Name = "RBDM";
	elem->Colour = COLPACK(0xCCCCCC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
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

	elem->Flammable = 1000;
	elem->Explosive = 1;
	elem->Meltable = 50;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 240;
	elem->Latent = 0;
	elem->Description = "铷。爆炸，特别是与水接触时。熔点低。";
	elem->DetailedDescription = "熔点：38.85℃/312K\n描述：低熔点，遇水爆炸，可与水(WATR)、蒸馏水(DSTW)、盐水(SLTW)、苏打水(BUBW)、酸(ACID)、火焰(FIRE)反应，可以导电而不爆炸。在 38.85℃熔化成液态铷。\n导热率：240\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 312.0f;
	elem->HighTemperatureTransitionElement = PT_LRBD;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &RBDM_init_element;
}
