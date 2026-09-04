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

int BMTL_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp>1)
	{
		parts[i].tmp--;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_METL || TYP(r)==PT_IRON) && RNG::Ref().chance(1, 100))
					{
						part_change_type(ID(r),x+rx,y+ry,PT_BMTL);
						parts[ID(r)].tmp = (parts[i].tmp<=7) ? parts[i].tmp = 1 : parts[i].tmp - RNG::Ref().between(0, 4);
					}
				}
	}
	else if (parts[i].tmp==1 && RNG::Ref().chance(1, 1000))
	{
		parts[i].tmp = 0;
		part_change_type(i,x,y,PT_BRMT);
	}
	return 0;
}

void BMTL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BMTL";
	elem->Name = "BMTL";
	elem->Colour = COLPACK(0x505070);
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "易碎金属。受压可熔化断裂。";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 1.0f;
	elem->HighPressureTransitionElement = ST;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1273.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &BMTL_update;
	elem->Graphics = NULL;
	elem->Init = &BMTL_init_element;
}
