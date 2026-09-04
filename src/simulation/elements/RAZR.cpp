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

void RAZR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RAZR";
	elem->Name = "RAZR";
	elem->Colour = COLPACK(0xC0C0C0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.07f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 1.5f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 500;

	elem->HeatConduct = 50;
	elem->Latent = 0;
	elem->Description = "沉重的银色粒子，会挤开一切。";
	elem->DetailedDescription = "描述：极重的银色致命粉末，重力和惯性都很强。移动时能挤开或穿过绝大多数可移动材料，尤其可穿过 CNCT 和 GEL，因此常用来切割、压碎或快速清理粉末与液体。\n性质：重力 1.5，重量 500；不燃烧、不熔化，也没有普通高低温或高低压相变。它仍会被不能移动的墙体和特殊不可破坏结构阻挡。\n导热率：50\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_PART|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &RAZR_init_element;
}
