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

int VINE_update(UPDATE_FUNC_ARGS)
{
	int rndstore = RNG::Ref().gen();
	int rx = (rndstore % 3) - 1;
	rndstore >>= 2;
	int ry = (rndstore % 3) - 1;
	rndstore >>= 2;
	if (rx || ry)
	{
		int r = pmap[y+ry][x+rx];
		if (!(rndstore % 15))
			part_change_type(i,x,y,PT_PLNT);
		else if (!r)
		{
			int np = sim->part_create(-1,x+rx,y+ry,PT_VINE);
			if (np<0) return 0;
			parts[np].temp = parts[i].temp;
			parts[i].tmp = 1;
			part_change_type(i,x,y,PT_PLNT);
		}
	}
	if (parts[i].temp > 350 && parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = (int)parts[i].temp;
	return 0;
}

int VINE_graphics(GRAPHICS_FUNC_ARGS)
{
	float maxtemp = std::max((float)cpart->tmp2, cpart->temp);
	if (maxtemp > 300)
	{
		*colr += (int)restrict_flt((maxtemp-300)/5,0,58);
		*colg -= (int)restrict_flt((maxtemp-300)/2,0,102);
		*colb += (int)restrict_flt((maxtemp-300)/5,0,70);
	}
	if (maxtemp < 273)
	{
		*colg += (int)restrict_flt((273-maxtemp)/4,0,255);
		*colb += (int)restrict_flt((273-maxtemp)/1.5f,0,255);
	}
	return 0;
}
void VINE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VINE";
	elem->Name = "VINE";
	elem->Colour = COLPACK(0x079A00);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 10;

	elem->Weight = 100;

	elem->HeatConduct = 65;
	elem->Latent = 0;
	elem->Description = "藤蔓，可以沿着WOOD生长。";
	elem->DetailedDescription = "描述：藤蔓，吸水生长，中心部分是植物(PLNT)，暴露在中子(NEUT)下会变成木材(WOOD)并向中子方向生长。藤蔓以一种半随机方式生长，形状有点像草。\n燃点：299.85℃/573K\n导热率：65\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 573.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->DefaultProperties.tmp = 1;

	elem->Update = &VINE_update;
	elem->Graphics = &VINE_graphics;
	elem->Init = &VINE_init_element;
}
