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

int FOG_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((sim->elements[TYP(r)].Properties & TYPE_SOLID) && RNG::Ref().chance(1, 10) && !parts[i].life &&
					!(sim->elements[TYP(r)].Properties & PROP_CLONE) && !(sim->elements[TYP(r)].Properties & PROP_BREAKABLECLONE))
				{
					part_change_type(i, x, y, PT_RIME);
				}
				if (TYP(r) == PT_SPRK)
				{
					parts[i].life += RNG::Ref().between(0, 19);
				}
				// GAS increases acidity
				if (TYP(r) == PT_GAS && parts[i].tmp < 10)
				{
					sim->part_kill(ID(r));
					if (parts[i].ctype == PT_DSTW)
						parts[i].ctype = 0;
					else
						parts[i].tmp++;
				}
			}
	return 0;
}

void FOG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FOG";
	elem->Name = "FOG";
	elem->Colour = COLPACK(0xAAAAAA);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 0.8f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.4f;
	elem->Loss = 0.70f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.99f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 1;

	elem->DefaultProperties.temp = 243.15f;
	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->Description = "雾，当电流通过 RIME 时产生。";
	elem->DetailedDescription = "描述：雾，原为隐藏元素，88.1 版本后可以直接制造，升温时(到达 100℃/373.15K)会变成水蒸气(WTRV)。\n制取方法：波义尔气(BOYL)和水(WATR)或者氧气(OXYG)混合时能产生雾(FOG)。霜(RIME)受到电脉冲刺激会形成雾(FOG)，但此方法制取的雾(FOG)会在 100 帧以后重新变成霜(RIME)。\n导热率：100\n初始温度：-30℃/243.15K";

	elem->Properties = TYPE_GAS|PROP_LIFE_DEC;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 373.15f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &FOG_update;
	elem->Graphics = NULL;
	elem->Init = &FOG_init_element;
}
