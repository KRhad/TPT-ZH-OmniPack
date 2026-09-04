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

void STNE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STNE";
	elem->Name = "STNE";
	elem->Colour = COLPACK(0xA0A0A0);
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

	elem->Weight = 90;

	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "石头。重颗粒，易熔。";
	elem->DetailedDescription = "描述：石头(STNE)是重粉末，加热后熔化为 LAVA。\n产生：冷却 LAVA，或对砖块(BRCK)施加足够压力。\n反应：ROCK 与 WATR 反应可生成 STNE。熔融 SLCN 与 OXYG 反应时有 1/3 概率生成 STNE，同时还会在 SAND、CLST/PQRT 等产物之间分配。\n熔点：709.85℃/983K\n导热率：150\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 983.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &STNE_init_element;
}
