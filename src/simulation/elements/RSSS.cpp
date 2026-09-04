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

int RSSS_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx < 2; rx++)
	{
		for (int ry = -1; ry < 2; ry++)
		{
			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			// Set RSSS ctype from nearby clone
			if ((TYP(r) == PT_CLNE) || (TYP(r) == PT_PCLN))
			{
				if (parts[ID(r)].ctype != PT_RSSS)
					parts[i].ctype = parts[ID(r)].ctype;
			}
			// Set RSSS tmp from nearby breakable clone
			if ((TYP(r) == PT_BCLN) || (TYP(r) == PT_PBCN))
			{
				if (parts[ID(r)].ctype != PT_RSSS)
					parts[i].tmp = parts[ID(r)].ctype;
			}
		}
	}

	//Block air like TTAN
	sim->air->blockair[y/CELL][x/CELL] = 1;
	sim->air->blockairh[y/CELL][x/CELL] = 0x8;

	return 0;
}

void RSSS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RSSS";
	elem->Name = "RSSS";
	elem->Colour = COLPACK(0xC43626);
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
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 130;
	elem->Latent = 0;
	elem->Description = "固化抗性材料。阻挡压力并绝缘；接触中子时液化。";
	elem->DetailedDescription = "描述：固态抗性材料。它像 TTAN 一样阻挡空气和压力传播，并在电路判定中像 INSL 一样隔断 SPRK；同时允许中子进入，不会主动导电。\n转化：中子与 RSSS 占据同一像素时会被吸收，并把 RSSS 液化。若 Ctype 有效，就转为 Ctype 指定元素；否则默认转为液态 RSST。若目标元素能携带 Ctype，则可用 Tmp 指定其 Ctype。\n参数获取：邻近 CLNE/PCLN 时复制其 Ctype；邻近 BCLN/PBCN 时把其 Ctype 复制到 Tmp。GRVT 进入 RSSS 后每帧有 1/5 概率被吸收。\n导热率：130\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NEUTPASS;
	elem->CarriesTypeIn = (1U << FIELD_CTYPE) | (1U << FIELD_TMP);

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &RSSS_update;
	elem->Graphics = NULL;
	elem->Init = &RSSS_init_element;
}
