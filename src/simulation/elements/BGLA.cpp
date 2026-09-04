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

void BGLA_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BGLA";
	elem->Name = "BGLA";
	elem->Colour = COLPACK(0x606060);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.3f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 5;
	elem->Hardness = 0;

	elem->Weight = 90;

	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "碎玻璃。玻璃受压破碎后形成的重质粒子；可熔化。还有贝果。";
	elem->DetailedDescription = "描述：碎玻璃，熔化后能重新变回玻璃(GLAS)。光子(PHOT)无法通过。\n熔点：1699.85℃/1973K\n制取方法：给玻璃(GLAS)加压或者加热液晶(LCRY)可以得到碎玻璃。玻璃(GLAS)被 DMG 破坏时也会产生碎玻璃。\n导热率：150\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART | PROP_PHOTPASS | PROP_NEUTPASS | PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1973.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &BGLA_init_element;
}
