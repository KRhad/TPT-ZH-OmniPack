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

int ROCK_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = (cpart->tmp2 - 7) * 6; // Randomized color noise based on tmp2
	*colr += z;
	*colg += z;
	*colb += z;
	
	if (cpart->temp >= 810.15) // Glows when hot, right before melting becomes bright
	{
		*pixel_mode |= FIRE_ADD;
		
		*firea = ((cpart->temp)-810.15)/45;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
	}
	return 0;
}

void ROCK_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp2 = RNG::Ref().between(0, 10);
}

void ROCK_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ROCK";
	elem->Name = "ROCK";
	elem->Colour = PIXPACK(0x727272);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 5;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 200;
	elem->Description = "固体，融化成各种元素。";
	elem->DetailedDescription = "描述：岩石(ROCK)是坚固固体，可作为混凝土(CNCT)的地基；CNCT 堆在 ROCK 上时不会从边缘滑落。ROCK 耐酸(ACID)、耐破坏炸药(DEST)，但仍会被高速水流缓慢侵蚀：水与周围速度差大于 0.5 时，每帧约有 1/1000 概率把 ROCK 变成 SAND(33%)或 STNE(67%)。\n熔融反应：多数反应只发生在熔融 ROCK 上。压力至少 25 P 时，每帧约有 1/12500 概率转化：\n25-50 P：BRMT 50%，CNCT 50%。\n50-73 P：QRTZ。\n73-75 P：GOLD 12.5%，QRTZ 87.5%。\n75-100 P 且温度至少 4726.85℃：TTAN 20%，IRON 80%。\n100 P 以上且温度至少 4726.85℃：另有 20% 概率生成放射性熔融物，分布为 URAN 20%、PLUT 16%、TUNG 64%。\n导热率：200\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 120;
	elem->HighPressureTransitionElement = PT_STNE;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1943.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Graphics = &ROCK_graphics;
	elem->Func_Create = &ROCK_create;
}
