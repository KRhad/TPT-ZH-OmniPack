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
#include "simulation/GolNumbers.h"

int CLNE_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !sim->elements[parts[i].ctype].Enabled)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
			{
				int r = photons[y+ry][x+rx];
				if (!r)
					r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				if (!(sim->elements[rt].Properties & PROP_CLONE) &&
					!(sim->elements[rt].Properties & PROP_BREAKABLECLONE) &&
					rt!=PT_STKM && rt!=PT_STKM2)
				{
					parts[i].ctype = rt;
					if (rt==PT_LIFE || rt==PT_LAVA)
						parts[i].tmp = parts[ID(r)].ctype;
				}
			}
	}
	else
	{
		if (parts[i].ctype == PT_LIFE)
			sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), PT_LIFE, parts[i].tmp);
		else if (parts[i].ctype != PT_LIGH || RNG::Ref().chance(1, 30))
		{
			int np = sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), TYP(parts[i].ctype));
			if (np>=0)
			{
				if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && sim->elements[parts[i].tmp].HighTemperatureTransitionElement==PT_LAVA)
					parts[np].ctype = parts[i].tmp;
			}
		}
	}
	return 0;
}

void CLNE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CLNE";
	elem->Name = "CLNE";
	elem->Colour = COLPACK(0xFFD010);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "克隆。复制它接触到的任何粒子。";
	elem->DetailedDescription = "描述：可以复制它第一个触碰到的物质，并将其记录到Ctype 值内。可以被 SING 和 VIRS 摧毁。它可以记住最后碰触到的物质并保存在图章/存档中。光子可以穿过所有类型的 CLNE 和 BCLN。\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_PHOTPASS | PROP_CLONE | PROP_NOCTYPEDRAW;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->CtypeDraw = &ctypeDrawVInTmp;
	elem->Init = &CLNE_init_element;
}
