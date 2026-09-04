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

int BOYL_update(UPDATE_FUNC_ARGS)
{
	float limit = parts[i].temp / 100;
	if (sim->air->pv[y / CELL][x / CELL] < limit)
		sim->air->pv[y / CELL][x / CELL] += 0.001f*(limit - sim->air->pv[y / CELL][x / CELL]);
	if (sim->air->pv[y / CELL + 1][x / CELL] < limit)
		sim->air->pv[y / CELL + 1][x / CELL] += 0.001f*(limit - sim->air->pv[y / CELL + 1][x / CELL]);
	if (sim->air->pv[y / CELL - 1][x / CELL] < limit)
		sim->air->pv[y / CELL - 1][x / CELL] += 0.001f*(limit - sim->air->pv[y / CELL - 1][x / CELL]);

	sim->air->pv[y / CELL][x / CELL + 1] += 0.001f*(limit - sim->air->pv[y / CELL][x / CELL + 1]);
	sim->air->pv[y / CELL + 1][x / CELL + 1] += 0.001f*(limit - sim->air->pv[y / CELL + 1][x / CELL + 1]);
	sim->air->pv[y / CELL][x / CELL - 1] += 0.001f*(limit - sim->air->pv[y / CELL][x / CELL - 1]);
	sim->air->pv[y / CELL - 1][x / CELL - 1] += 0.001f*(limit - sim->air->pv[y / CELL - 1][x / CELL - 1]);

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_WATR)
				{
					if (RNG::Ref().chance(1, 30))
						part_change_type(ID(r),x+rx,y+ry,PT_FOG);
				}
				else if (TYP(r)==PT_O2)
				{
					if (RNG::Ref().chance(1, 9))
					{
						sim->part_kill(ID(r));
						part_change_type(i, x, y, PT_WATR);
						sim->air->pv[y/CELL][x/CELL] += 4.0;
						return 1;
					}
				}
			}
	return 0;
}

void BOYL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BOYL";
	elem->Name = "BOYL";
	elem->Colour = COLPACK(0x0A3200);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.18f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP + 2.0f + 273.15f;
	elem->HeatConduct = 42;
	elem->Latent = 0;
	elem->Description = "波义耳，可变压力气体。受热时膨胀。";
	elem->DetailedDescription = "描述：不可燃气体，热胀冷缩。也可用于核反应堆，在容器内放入铀(URAN)和波义尔气，铀会在压力下产生大量热，而热量又使波义尔气膨胀产生更高压力，因此这个反应就能一直进行下去。波义耳气和氧气(OXYG)反应能生成水(WATR)，和水反应能生成雾(FOG)。\n导热率：42\n初始温度：24.00℃/297.15K";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BOYL_update;
	elem->Graphics = NULL;
	elem->Init = &BOYL_init_element;
}
