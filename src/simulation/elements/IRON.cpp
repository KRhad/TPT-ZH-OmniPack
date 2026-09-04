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

int IRON_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life)
		return 0;
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				switch (TYP(r))
				{
				case PT_SALT:
					if (RNG::Ref().chance(1, 47))
						goto succ;
					break;
				case PT_SLTW:
					if (RNG::Ref().chance(1, 67))
						goto succ;
					break;
				case PT_WATR:
					if (RNG::Ref().chance(1, 1200))
						goto succ;
					break;
				case PT_O2:
					if (RNG::Ref().chance(1, 250))
						goto succ;
					break;
				case PT_LO2:
					goto succ;
				default:
					break;
				}
			}
	return 0;
succ:
	sim->part_change_type(i, x, y, PT_BMTL);
	parts[i].tmp = RNG::Ref().between(20, 29);
	return 0;
}

void IRON_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_IRON";
	elem->Name = "IRON";
	elem->Colour = COLPACK(0x707070);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 49;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "铁。接触盐会生锈，可用于电解 WATR。";
	elem->DetailedDescription = "描述：铁(IRON)会被盐(SALT)、盐水(SLTW)、氧气(OXYG)、水(WATR)和液氧(LOXY)腐蚀，并逐步转化为脆金属(BMTL)，可用于电解水相关装置。\n腐蚀过程：IRON 长时间接触上述物质后先变成 BMTL，继续暴露会变成金属粉(BRMT)，表示进一步锈蚀。附近放置 GOLD 可逆转并阻止这一过程。\n熔点：1413.85℃/1687K\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1687.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &IRON_update;
	elem->Graphics = NULL;
	elem->Init = &IRON_init_element;
}
