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

int O2_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if(parts[ID(r)].tmp&1&&TYP(r)==PT_PLSM)
					continue;

				if (TYP(r)==PT_FIRE)
				{
					parts[ID(r)].temp += RNG::Ref().between(0, 99);
					if (parts[ID(r)].tmp & 0x01)
						parts[ID(r)].temp=3473;
					parts[ID(r)].tmp |= 2;

					sim->part_create(i,x,y,PT_FIRE);
					parts[i].temp += RNG::Ref().between(0, 99);
					parts[i].tmp |= 2;
				}
				else if (TYP(r)==PT_PLSM && !(parts[ID(r)].tmp&4))
				{
					sim->part_create(i,x,y,PT_FIRE);
					parts[i].temp += RNG::Ref().between(0, 99);
					parts[i].tmp |= 2;
				}
			}

	if (parts[i].temp > 9973.15 && sim->air->pv[y/CELL][x/CELL] > 250.0f)
	{
		int gravPos = ((y/CELL)*(XRES/CELL))+(x/CELL);
		float o2_gravx = sim->grav->gravx[gravPos];
		float o2_gravy = sim->grav->gravy[gravPos];
		if (o2_gravx*o2_gravx + o2_gravy*o2_gravy > 400)
		{
			if (RNG::Ref().chance(1, 5))
			{
				int j;
				sim->part_create(i,x,y,PT_BRMT);
	
				j = sim->part_create(-3,x,y,PT_NEUT);
				if (j != -1)
					parts[j].temp = MAX_TEMP;
				j = sim->part_create(-3,x,y,PT_PHOT);
				if (j != -1)
				{
					parts[j].temp = MAX_TEMP;
					parts[j].tmp = 0x1;
				}
				int rx = x + RNG::Ref().between(-1, 1), ry = y + RNG::Ref().between(-1, 1), rt = TYP(pmap[ry][rx]);
				if (sim->can_move[PT_PLSM][rt] || rt == PT_O2)
				{
					j = sim->part_create(-3,rx,ry,PT_PLSM);
					if (j > -1)
					{
						parts[j].temp = MAX_TEMP;
						parts[j].tmp |= 4;
					}
				}
				j = sim->part_create(-3,x,y,PT_GRVT);
				if (j != -1)
					parts[j].temp = MAX_TEMP;
	
				parts[i].temp = MAX_TEMP;
				sim->air->pv[y/CELL][x/CELL] = 256;
			}
		}
	}
	return 0;
}

void O2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_O2";
	elem->Name = "OXYG";
	elem->Colour = COLPACK(0x80A0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 3.0f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "氧气。容易助燃。";
	elem->DetailedDescription = "描述：氧气(OXYG)是高度助燃的气体，可被 FIRE 点燃；低温或压力超过 100 P 时会液化成液氧(LOXY)。植物(PLNT)吸收 SMKE 或 CO2 时可产生 OXYG，用于模拟光合作用。OXYG 与 BOYL 反应会产生 WTRV 和约 4 P 压力。\n聚变：在极强牛顿重力、9700℃以上高温及 250 P 以上压力下，OXYG 会聚变为熔融脆金属(BMTL)，并各产生一个 PHOT、PLSM 和 GRVT。\n液化点：-183.15℃/90K\n导热率：70\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 90.0f;
	elem->LowTemperatureTransitionElement = PT_LO2;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &O2_update;
	elem->Graphics = NULL;
	elem->Init = &O2_init_element;
}
