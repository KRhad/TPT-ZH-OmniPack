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

int FRZZ_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_WATR && RNG::Ref().chance(1, 20))
				{
					part_change_type(ID(r),x+rx,y+ry,PT_FRZW);
					parts[ID(r)].life = 100;
					sim->part_kill(i);
					return 1;
				}
			}
	return 0;
}

void FRZZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FRZZ";
	elem->Name = "FRZZ";
	elem->Colour = COLPACK(0xC0E0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.90f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.05f;
	elem->Diffusion = 0.01f;
	elem->HotAir = -0.00005f* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 50;

	elem->DefaultProperties.temp = 253.15f;
	elem->HeatConduct = 46;
	elem->Latent = 0;
	elem->Description = "冷冻粉。熔化后形成会持续降温的冰，并可随普通水扩散。";
	elem->DetailedDescription = "描述：轻粉末，很冷，能立即冻住水。能将水(WATR)转变成寒水(FRZW)，温度低于-223.15℃/50K 时变为可以自动降温的冰(ICE)，当温度高于零度时变成寒水。寒水能将其他水变成寒水。\n熔点：0℃/273.15K\n转变温度：-223.15℃/50K\n压力极限：1.8 P，变为雪(SNOW)\n导热率：46\n初始温度：-20℃/253.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 1.8f;
	elem->HighPressureTransitionElement = PT_SNOW;
	elem->LowTemperatureTransitionThreshold = 50.0f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 273.15f;
	elem->HighTemperatureTransitionElement = PT_FRZW;

	elem->Update = &FRZZ_update;
	elem->Graphics = NULL;
	elem->Init = &FRZZ_init_element;
}
