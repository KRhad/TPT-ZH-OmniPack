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

int COAL_graphics(GRAPHICS_FUNC_ARGS);

int BCOL_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life <= 0)
	{
		sim->part_create(i, x, y, PT_FIRE);
		return 1;
	}
	else if (parts[i].life < 100)
	{
		parts[i].life--;
		sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), PT_FIRE);
	}

	/*if(100-parts[i].life > parts[i].tmp2)
		parts[i].tmp2 = 100-parts[i].life;
	if(parts[i].tmp2 < 0) parts[i].tmp2 = 0;
	for (int trade = 0; trade<4; trade ++)
	{
		int rx = RNG::Ref().between(-2, 2);
		int ry = RNG::Ref().between(-2, 2);
		if (rx || ry)
		{
			r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if ((TYP(r)==PT_COAL || TYP(r)==PT_BCOL)&&(parts[i].tmp2>parts[ID(r)].tmp2)&&parts[i].tmp2>0)//diffusion
			{
				int temp = parts[i].tmp2 - parts[ID(r)].tmp2;
				if(temp < 10)
					continue;
				if (temp ==1)
				{
					parts[ID(r)].tmp2 ++;
					parts[i].tmp2 --;
				}
				else if (temp>0)
				{
					parts[ID(r)].tmp2 += temp/2;
					parts[i].tmp2 -= temp/2;
				}
			}
		}
	}*/
	if(parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = (int)parts[i].temp;
	return 0;
}

void BCOL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BCOL";
	elem->Name = "BCOL";
	elem->Colour = COLPACK(0x333333);
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
	elem->Meltable = 0;
	elem->Hardness = 2;
	elem->PhotonReflectWavelengths = 0x00000000;

	elem->Weight = 90;

	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "碎煤。颗粒重，燃烧缓慢。";
	elem->DetailedDescription = "描述：重粉末，只能用明火点燃，缓慢燃烧。被中子(NEUT)撞击时有概率变成锯末(SAWD)。\n产生：BCOL 是由 WOOD 在低压(小于-10)和高温(大于 499.85℃)下生产的。\n导热率：150\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 110;

	elem->Update = &BCOL_update;
	elem->Graphics = &COAL_graphics;
	elem->Init = &BCOL_init_element;
}
