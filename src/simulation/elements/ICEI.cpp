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

int ICE_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].ctype == PT_FRZW)//get colder if it is from FRZW
	{
		parts[i].temp = restrict_flt(parts[i].temp-1.0f, MIN_TEMP, MAX_TEMP);
	}
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_SALT || TYP(r)==PT_SLTW)
				{
					if (parts[i].temp > sim->elements[PT_SLTW].LowTemperatureTransitionThreshold && RNG::Ref().chance(1, 200))
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						sim->part_change_type(ID(r), x+rx, y+ry, PT_SLTW);
						return 0;
					}
				}
				else if ((TYP(r)==PT_FRZZ) && RNG::Ref().chance(1, 200))
				{
					sim->part_change_type(ID(r),x+rx,y+ry,PT_ICEI);
					parts[ID(r)].ctype = PT_FRZW;
				}
			}
	return 0;
}

void ICEI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ICEI";
	elem->Name = "ICE";
	elem->Colour = COLPACK(0xA0C0FF);
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
	elem->HotAir = -0.0003f* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP - 50.0f + 273.15f;
	elem->HeatConduct = 46;
	elem->Latent = 1095;
	elem->Description = "在压力下破碎。冷却空气。";
	elem->DetailedDescription = "描述：固体，冷冻的水，在压力下会破碎变成雪(SNOW)。可以熔化。可以使中子(NEUT)减速。它的 Ctype 值决定了它会融化成什么。例如，Ctype 值为 SLTW 的 ICE 在融化时会变成 SLTW。\n熔点：0℃/273.15K\n压力极限：0.8 P\n导热率：46\n初始温度：-28.00℃/245.15K";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_NEUTPASS;
	elem->CarriesTypeIn = (1U << FIELD_CTYPE);

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 0.8f;
	elem->HighPressureTransitionElement = PT_SNOW;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 252.05f;
	elem->HighTemperatureTransitionElement = ST;

	elem->DefaultProperties.ctype = PT_WATR;

	elem->Update = &ICE_update;
	elem->Graphics = NULL;
	elem->Init = &ICEI_init_element;
}
