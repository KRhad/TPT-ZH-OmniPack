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

int CLST_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_WATR)
				{
					if (RNG::Ref().chance(1, 1500))
					{
						sim->part_create(i, x, y, PT_PSTS);
						sim->part_kill(ID(r));
					}
				}
				else if (TYP(r) == PT_NITR)
				{
					sim->part_create(i, x, y, PT_BANG);
					sim->part_create(ID(r), x+rx, y+ry, PT_BANG);
				}
				else if (TYP(r) == PT_CLST)
				{
					float cxy = 0;
					if (parts[i].temp < 195)
						cxy = 0.05f;
					else if (parts[i].temp < 295)
						cxy = 0.015f;
					else if (parts[i].temp < 350)
						cxy = 0.01f;
					else
						cxy = 0.005f;
					parts[i].vx += cxy*rx;
					parts[i].vy += cxy*ry;//These two can be set not to calculate over 350 later. They do virtually nothing over 0.005.
				}
			}
	return 0;
}

int CLST_graphics(GRAPHICS_FUNC_ARGS)
{
	// speckles!
	int z = (cpart->tmp - 5) * 16;
	*colr += z;
	*colg += z;
	*colb += z;
	return 0;
}

void CLST_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = RNG::Ref().between(0, 6);
}

void CLST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CLST";
	elem->Name = "CLST";
	elem->Colour = COLPACK(0xE4A4A4);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.2f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 2;
	elem->Hardness = 2;

	elem->Weight = 55;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "黏土粉尘。与水混合会形成糊状物。";
	elem->DetailedDescription = "描述：和水结合时产生浆糊(PSTE)。它能自然的结合在一起，温度越低越牢固，在大约-70℃时冻结，顶部就像混凝土一样牢固。CLST 与 NITR 混合时会产生 TNT。\n产生：CLST 可以通过将 CRMC 置于低于-30 P 的压力下来创建。\nPSTE + GEL → CLST + GEL\nPSTE + SPNG → CLST + SPNG\n3×SLCN(熔融) + 3×OXYG → SAND + STNE +CLST/PQRT\n反应：WATR + CLST → PSTS\nCLST(熔融) + PQRT(熔融)/QRTZ(熔融) → 2×CRMC(熔融)\nNITR + CLST → TNT\n熔点：982.85℃/1256K\n导热率：70\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1256.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &CLST_update;
	elem->Graphics = &CLST_graphics;
	elem->Func_Create = &CLST_create;
	elem->Init = &CLST_init_element;
}
