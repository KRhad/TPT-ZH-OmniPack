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

int SLTW_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				switch (TYP(r))
				{
				case PT_SALT:
					if (RNG::Ref().chance(1, 2000))
						part_change_type(ID(r), x+rx, y+ry, PT_SLTW);
					break;
				case PT_PLNT:
					if (RNG::Ref().chance(1, 40))
						sim->part_kill(ID(r));
					break;
				case PT_RBDM:
				case PT_LRBD:
					if ((legacy_enable || parts[i].temp>(273.15f+12.0f)) && RNG::Ref().chance(1, 100))
					{
						part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 4;
						parts[i].ctype = PT_WATR;
					}
					break;
				case PT_FIRE:
					if (parts[ID(r)].ctype != PT_WATR)
					{
						sim->part_kill(ID(r));
						if (RNG::Ref().chance(1, 30))
						{
							sim->part_kill(i);
							return 1;
						}
					}
					break;
				case PT_NONE:
					break;
				default:
					continue;
				}
			}
	return 0;
}

void SLTW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SLTW";
	elem->Name = "SLTW";
	elem->Colour = COLPACK(0x4050F0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 35;

	elem->HeatConduct = 75;
	elem->Latent = 7500;
	elem->Description = "盐水，导电，不易结冰。";
	elem->DetailedDescription = "描述：盐(SALT)+水(WATR)的产物，能更快的导电(比 WATR 快)，具有更高沸点和更低的凝固点，沸腾后产生水蒸气(WTRV)+盐(SALT)。同时它还会使植物(PLNT)死亡和使石英(QRTZ)缓慢生长。SLTW 在 T ≥ 109.86 + 2 P。承受任何压力的 SLTW 在 ≤ -20.01 摄氏度时会变成 ICE (Ctype SLTW) 或 SNOW，如果压力< 0.8 P 则变成 ICE (Ctype SLTW)，如果压力≥ 0.8 P 时会变成 SNOW。\n沸点：109.85℃/383K\n凝固点：-21.1℃/252.05K\n导热率：75\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_LIQUID | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 252.05f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 383.0f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &SLTW_update;
	elem->Graphics = NULL;
	elem->Init = &SLTW_init_element;
}
