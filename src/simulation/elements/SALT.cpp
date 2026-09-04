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

void SALT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SALT";
	elem->Name = "SALT";
	elem->Colour = COLPACK(0xFFFFFF);
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
	elem->Hardness = 1;

	elem->Weight = 75;

	elem->HeatConduct = 110;
	elem->Latent = 0;
	elem->Description = "盐，溶于水。";
	elem->DetailedDescription = "描述：能溶于水(WATR)形成盐水(SLTW)，较高温度下能熔化，能腐蚀铁(IRON)变成脆金属(BMTL)和金属粉(BRMT)。\n反应：可以将 WATR 和 DSTW 变成 SLTW，同时慢慢溶解。将 IRON 转换为 BMTL 然后是 BRMT，除非 GOLD 就在附近以将其还原。SALT 也会慢慢溶解在 SLTW 中，这两者都会破坏 PLNT。\n凝胶(SPNG)和海绵(GEL)从盐水中吸收水分，平均每四个盐水(SLTW)颗粒产生一个盐(SALT)。\n熔点：899.85℃/1173K\n导热率：110\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1173.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &SALT_init_element;
}
