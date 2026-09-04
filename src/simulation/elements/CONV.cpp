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

int CONV_update(UPDATE_FUNC_ARGS)
{
	int ctype = TYP(parts[i].ctype), ctypeExtra = ID(parts[i].ctype);
	if (ctype <= 0 || !sim->elements[ctype].Enabled || ctype == PT_CONV)
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
				if (!(sim->elements[rt].Properties&PROP_CLONE) && !(sim->elements[rt].Properties&PROP_BREAKABLECLONE) &&
					rt != PT_STKM && rt != PT_STKM2 && rt != PT_CONV)
				{
					parts[i].ctype = rt;
					if (rt == PT_LIFE)
						parts[i].ctype |= PMAPID(parts[ID(r)].ctype);
				}
			}
	}
	else
	{
		int restrictElement = sim->IsElement(parts[i].tmp) ? parts[i].tmp : 0;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
			{
				int r = photons[y+ry][x+rx];
				if (!r || (restrictElement && ((TYP(r) == restrictElement) == (parts[i].tmp2 == 1))))
					r = pmap[y+ry][x+rx];
				if (!r || (restrictElement && ((TYP(r) == restrictElement) == (parts[i].tmp2 == 1))))
					continue;
				if (TYP(r) != PT_CONV && !(sim->elements[TYP(r)].Properties&PROP_INDESTRUCTIBLE) && TYP(r) != ctype)
				{
					sim->part_create(ID(r), x+rx, y+ry, ctype, ctypeExtra);
				}
			}
	}
	return 0;
}

void CONV_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CONV";
	elem->Name = "CONV";
	elem->Colour = COLPACK(0x0AAB0A);
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
	elem->Description = "转换器。把一切转化为它最先接触的元素。";
	elem->DetailedDescription = "描述：固体，可以转换它接触到的物质的 Type 值为它自身的 Ctype 值，使用方法类似复制体(CLNE)。可以通过Tmp 值可设定待转化原料物质。当 Tmp2 值为 1 时，只会转换 Tmp 以外的元素。例如，如果 CONV(WATR)的Tmp 为 CNCT，Tmp2 为 1，它会将除 CNCT 之外的所有内容都转换为 WATR。\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NOCTYPEDRAW;
	elem->CarriesTypeIn = (1U << FIELD_CTYPE) | (1U << FIELD_TMP);

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &CONV_update;
	elem->Graphics = NULL;
	elem->CtypeDraw = &ctypeDrawVInCtype;
	elem->Init = &CONV_init_element;
}
