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

int GRVT_update(UPDATE_FUNC_ARGS)
{
	// At higher tmps they just go completely insane
	if (sim->parts[i].tmp >= 100)
		sim->parts[i].tmp = 100;
	if (sim->parts[i].tmp <= -100)
		sim->parts[i].tmp = -100;

	int under = pmap[y][x];
	int utype = TYP(under);

	//Randomly kill GRVT inside RSSS
	if((utype == PT_RSSS) && RNG::Ref().chance(1, 5))
	{
		sim->part_kill(i);
		return 1;
	}

	sim->grav->gravmap[(y / CELL) * (XRES / CELL) + (x / CELL)] = 0.2f * sim->parts[i].tmp;
	return 0;
}

int GRVT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 5;
	*firer = 0;
	*fireg = 250;
	*fireb = 170;

	*pixel_mode |= FIRE_BLEND;
	return 1;
}

void GRVT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = RNG::Ref().between(0, 359) * 3.14159f / 180.0f;
	sim->parts[i].life = 250 + RNG::Ref().between(0, 199);
	sim->parts[i].vx = 2.0f * cosf(a);
	sim->parts[i].vy = 2.0f * sinf(a);
}

void GRVT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GRVT";
	elem->Name = "GRVT";
	elem->Colour = COLPACK(0x00EE76);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = -1;

	elem->HeatConduct = 61;
	elem->Latent = 0;
	elem->Description = "引力子。创建牛顿引力。";
	elem->DetailedDescription = "描述：玻色子之一，会在周围产生额外的引力场。GRVT 可以穿过大多数材料，类似于PROT。但它不会改变不导热的材料的温度。\n计算公式：周围产生的引力场=0.2*Tmp 值，Tmp 值初始为 7\n导热率：61\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = 7;

	elem->Update = &GRVT_update;
	elem->Graphics = &GRVT_graphics;
	elem->Func_Create = &GRVT_create;
	elem->Init = &GRVT_init_element;
}
