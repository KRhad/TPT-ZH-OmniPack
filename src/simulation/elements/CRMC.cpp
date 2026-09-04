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

int CRMC_update(UPDATE_FUNC_ARGS)
{
	float origTemp = parts[i].temp;
	if (sim->air->pv[y/CELL][x/CELL] < -30.0f)
	{
		sim->part_create(i, x, y, PT_CLST);
		parts[i].temp = origTemp;
	}
	return 0;
}

int CRMC_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = (cpart->tmp2 - 2) * 8;
	*colr += z;
	*colg += z;
	*colb += z;
	return 0;
}

void CRMC_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp2 = RNG::Ref().between(0, 4);
}

void CRMC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CRMC";
	elem->Name = "CRMC";
	elem->Colour = COLPACK(0xD6D1D4);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 35;
	elem->Latent = 0;
	elem->Description = "陶瓷。在压力下变得更强。";
	elem->DetailedDescription = "描述：固体，受压时熔点会增加。允许中子(NEUT)、引力子(GRVT)以及质子(PROT)通过，且对 ACID 免疫。在一定的负压下(≤-30 P)会转变成粘土砂(CLST)。通过对 5 个位置(包括本身在内每个方向延伸两个像素)压力取平均值。\n制取方法：熔融的石英(QRTZ)+熔融的粘土砂(CLST)。\n熔点：0 P，2614.00℃/2887.15K，压力每升高 1 P 熔点升高 10℃，255 P 下熔点为 5164℃。\n导热率：35\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2887.15f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &CRMC_update;
	elem->Graphics = &CRMC_graphics;
	elem->Func_Create = &CRMC_create;
	elem->Init = &CRMC_init_element;
}
