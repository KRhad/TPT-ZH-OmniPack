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

int RIME_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_SPRK)
				{
					part_change_type(i, x, y, PT_FOG);
					parts[i].life = RNG::Ref().between(60, 109);
				}
				else if (TYP(r) == PT_FOG && parts[ID(r)].life > 0)
				{
					part_change_type(i, x, y, PT_FOG);
					parts[i].life = parts[ID(r)].life;
				}
				// GAS increases acidity
				else if (TYP(r) == PT_GAS && parts[i].tmp < 10)
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

void RIME_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RIME";
	elem->Name = "RIME";
	elem->Colour = COLPACK(0xCCCCCC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.00f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.00f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 32;

	elem->Weight = 100;

	elem->DefaultProperties.temp = -30.0f + 273.15f;
	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->Description = "霜。蒸汽快速冷却、跳过液态直接凝华时形成。";
	elem->DetailedDescription = "描述：霜，可以通电升华成雾(FOG)。0 摄氏度或更高温度时会变回 WATR，或在某些压力下变回 WTRV。\n熔点：0℃/273.15K\n制取方法：如果水蒸气快速冷却凝华就有可能形成霜(RIME)。\n导热率：100\n初始温度：-30℃/243.15K";

	elem->Properties = TYPE_SOLID;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 273.15f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &RIME_update;
	elem->Graphics = NULL;
	elem->Init = &RIME_init_element;
}
