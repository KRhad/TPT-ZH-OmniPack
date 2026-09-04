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

int AMTR_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				// Would a table lookup be faster than 11 checks?
				if (rt != PT_AMTR && !(sim->elements[rt].Properties&PROP_INDESTRUCTIBLE) && !(sim->elements[rt].Properties&PROP_CLONE)
				        && rt != PT_NONE && rt != PT_VOID && rt != PT_BHOL && rt != PT_NBHL && rt != PT_PRTI && rt != PT_PRTO)
				{
#ifndef NOMOD
					if (rt == PT_PPTI || rt == PT_PPTO)
						continue;
#endif
					if (!parts[i].ctype || (parts[i].ctype == TYP(r)) != (parts[i].tmp&1))
					{
						parts[i].life++;
						if (parts[i].life == 4)
						{
							sim->part_kill(i);
							return 1;
						}
						if (RNG::Ref().chance(1, 10))
							sim->part_create(ID(r), x+rx, y+ry, PT_PHOT);
						else
							sim->part_kill(ID(r));
						sim->air->pv[y/CELL][x/CELL] -= 2.0f;
					}
				}
			}
	return 0;
}

int AMTR_graphics(GRAPHICS_FUNC_ARGS)
{
	// don't render AMTR as a gas
	// this function just overrides the default graphics
	return 1;
}

void AMTR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_AMTR";
	elem->Name = "AMTR";
	elem->Colour = COLPACK(0x808080);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.00f;
	elem->Gravity = 0.10f;
	elem->Diffusion = 1.00f;
	elem->HotAir = 0.0000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "反物质。可湮灭大多数粒子。";
	elem->DetailedDescription = "描述：可以破坏大多数物质并产生低压和光子，受到微弱的重力影响。除AMTR、DMND、CLNE、PCLN、VOID、VACU、BHOL、PRTI、PRTO、所有能量粒子不可破坏。\n元素参数：Life 值代表反应次数，初始值为 0 每与另一个粒子发生反应(包括产生光子)时，Life 值增加 1，当 Life值到达 4 时便会消失。\n导热率：70\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &AMTR_update;
	elem->Graphics = &AMTR_graphics;
	elem->Init = &AMTR_init_element;
}
