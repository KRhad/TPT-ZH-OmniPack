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

int THDR_update(UPDATE_FUNC_ARGS)
{
	bool kill = false;
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				if ((sim->elements[rt].Properties & PROP_CONDUCTS) && parts[ID(r)].life == 0 && !(rt == PT_WATR || rt == PT_SLTW) && parts[ID(r)].ctype != PT_SPRK)
				{
					sim->spark_conductive(ID(r), x + rx, y + ry);
					kill = true;
				}
				else if (rt != PT_CLNE && rt != PT_THDR && rt != PT_SPRK && !(sim->elements[rt].Properties & PROP_INDESTRUCTIBLE) && rt != PT_FIRE)
				{
					sim->air->pv[y/CELL][x/CELL] = restrict_flt(sim->air->pv[y/CELL][x/CELL] + 100.0f, MIN_PRESSURE, MAX_PRESSURE);
					if (legacy_enable && RNG::Ref().chance(1, 200))
					{
						parts[i].life = RNG::Ref().between(120, 169);
						part_change_type(i, x, y, PT_FIRE);
					}
					else
					{
						kill = true;
					}
				}
			}
	if (kill)
	{
		sim->part_kill(i);
		return 1;
	}
	return 0;
}

int THDR_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 160;
	*fireg = 192;
	*fireb = 255;
	*firer = 144;
	*pixel_mode |= FIRE_ADD;
	return 1;
}

void THDR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_THDR";
	elem->Name = "THDR";
	elem->Colour = COLPACK(0xFFFFA0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.0f;
	elem->Loss = 0.30f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.6f;
	elem->Diffusion = 0.62f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = 9000.0f + 273.15f;
	elem->HeatConduct = 1;
	elem->Latent = 0;
	elem->Description = "闪电！非常热，会对大多数材料造成损坏，并将电流传输到金属。";
	elem->DetailedDescription = "描述：球状闪电(THDR)是温度约 9000℃的带电类液体粒子，运动不受空气压力影响。接触物质时会产生约 256 P 的强烈压力冲击波，并把高温传给非金属或刚结束 SPRK、暂时不能导电的金属。\n用途：可提供启动聚变所需的瞬时压力和热量，例如与 HYGN 配合。但其导热率极低，不适合持续加热。\n导热率：1\n初始温度：9000℃/9273.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &THDR_update;
	elem->Graphics = &THDR_graphics;
	elem->Init = &THDR_init_element;
}
