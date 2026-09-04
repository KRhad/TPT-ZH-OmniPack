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

int WOOD_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 450 && parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = (int)parts[i].temp;

	if (parts[i].temp > 773.0f && sim->air->pv[y/CELL][x/CELL] <= -10.0f)
	{
		float temp = parts[i].temp;
		sim->part_create(i, x, y, PT_BCOL);
		parts[i].temp = temp;
	}

	return 0;
}

int WOOD_graphics(GRAPHICS_FUNC_ARGS)
{
	float maxtemp = std::max((float)cpart->tmp, cpart->temp);
	if (maxtemp > 400)
	{
		*colr -= (int)restrict_flt((maxtemp-400)/3,0,172);
		*colg -= (int)restrict_flt((maxtemp-400)/4,0,140);
		*colb -= (int)restrict_flt((maxtemp-400)/20,0,44);
		if (maxtemp > 450)
			cpart->tmp = (int)maxtemp;
	}
	if (maxtemp < 273)
	{
		*colr -= (int)restrict_flt((273-maxtemp)/5,0,40);
		*colg += (int)restrict_flt((273-maxtemp)/4,0,40);
		*colb += (int)restrict_flt((273-maxtemp)/1.5f,0,150);
	}
	return 0;
}
void WOOD_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WOOD";
	elem->Name = "WOOD";
	elem->Colour = COLPACK(0xC0A040);
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

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 16;

	elem->Weight = 100;

	elem->HeatConduct = 164;
	elem->Latent = 0;
	elem->Description = "木头，易燃。";
	elem->DetailedDescription = "描述：固体，可燃物，允许中子(NEUT)通过。以一般速度燃烧。在高温下，WOOD 会变暗，直到变成煤黑色，然后燃烧。另一方面，如果低于冰点，木材会变成冰蓝色。烧焦的木材(超过 176.85℃)永远无法恢复并恢复其正常颜色。NEUT 可以穿过木材，同时使木材变形。当 VINE 放在 WOOD 旁边时，VINE 将在 WOOD 上生长，直到所有可以到达的木材都被 PLNT/VINE 覆盖。NEUT 穿过 PLNT 时会将其变 WOOD。当高速粒子撞击 WOOD 时，它会变成 SAWD。木材在低压(-10 P)和高温(499.85℃)下会变成 BCOL\n燃点：599.85℃/873K\n导热率：164\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 873.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &WOOD_update;
	elem->Graphics = &WOOD_graphics;
	elem->Init = &WOOD_init_element;
}
