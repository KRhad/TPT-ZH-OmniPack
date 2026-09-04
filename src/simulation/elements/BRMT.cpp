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

int BRMT_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 523.15f)//250.0f+273.15f
	{
		int tempFactor = 1000 - (int)((523.15f-parts[i].temp)*2);
		if(tempFactor < 2)
			tempFactor = 2;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (TYP(r) == PT_BREL && RNG::Ref().chance(1, tempFactor))
					{
						if (RNG::Ref().chance(1, 2))
						{
							sim->part_create(ID(r), x+rx, y+ry, PT_THRM);
						}
						else
							sim->part_create(i, x, y, PT_THRM);
						//part_change_type(ID(r),x+rx,y+ry,PT_BMTL);
						//parts[ID(r)].tmp = (parts[i].tmp<=7) ? parts[i].tmp = 1 : parts[i].tmp - RNG::Ref().between(0, 4);
					}
				}
	}
	return 0;
}

void BRMT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BRMT";
	elem->Name = "BRMT";
	elem->Colour = COLPACK(0x705060);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.3f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 2;
	elem->Hardness = 2;

	elem->Weight = 90;

	elem->HeatConduct = 211;
	elem->Latent = 0;
	elem->Description = "金属碎片。由铁锈蚀或金属受压破裂形成。";
	elem->DetailedDescription = "描述：重粉末，能导电。如果脆金属(BMTL)熔化后缓慢冷却就会形成金属粉。在电脉冲(SPRK)通过时会升温。被腐蚀的铁(IRON)或者脆金属也能形成金属粉。另一种方式是将脆金属暴露在高压下使之碎裂。在>250℃ 下与 BREL结合时形成铝热剂 THRM，燃烧铝热剂能形成熔融态的脆金属(熔融 BMTL)。\n导热率：211\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1273.0f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &BRMT_update;
	elem->Graphics = NULL;
	elem->Init = &BRMT_init_element;
}
