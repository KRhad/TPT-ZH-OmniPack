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

int STOR_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp && !sim->IsElement(parts[i].tmp))
		parts[i].tmp = 0;
	if (parts[i].life && !parts[i].tmp)
		parts[i].life--;
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					r = photons[y+ry][x+rx];
				if (!r)
					continue;
				if (!parts[i].tmp && !parts[i].life && TYP(r) != PT_STOR && !(sim->elements[TYP(r)].Properties & TYPE_SOLID) && (!parts[i].ctype || TYP(r) == parts[i].ctype))
				{
					parts[i].tmp = parts[ID(r)].type;
					parts[i].temp = parts[ID(r)].temp;
					parts[i].tmp2 = parts[ID(r)].life;
					parts[i].tmp3 = parts[ID(r)].tmp;
					parts[i].tmp4 = parts[ID(r)].ctype;
					sim->part_kill(ID(r));
				}
				if (parts[i].tmp && TYP(r) == PT_SPRK && parts[ID(r)].ctype == PT_PSCN && parts[ID(r)].life > 0 && parts[ID(r)].life < 4)
				{
					for (int ry1 = 1; ry1 >= -1; ry1--)
					{
						// Oscilate the X starting at 0, 1, -1, 3, -5, etc (Though stop at -1)
						for (int rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1 - rx1 + 1)
						{
							int np = sim->part_create(-1, x + rx1, y + ry1, TYP(parts[i].tmp));
							if (np != -1)
							{
								parts[np].temp = parts[i].temp;
								parts[np].life = parts[i].tmp2;
								parts[np].tmp = parts[i].tmp3;
								parts[np].ctype = parts[i].tmp4;
								parts[i].tmp = 0;
								parts[i].life = 10;
								break;
							}
						}
					}
				}
			}
	return 0;
}

int STOR_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp)
	{
		*pixel_mode |= PMODE_GLOW;
		*colr = 0x50;
		*colg = 0xDF;
		*colb = 0xDF;
	}
	else
	{
		*colr = 0x20;
		*colg = 0xAF;
		*colb = 0xAF;
	}
	return 0;
}

bool STOR_ctypeDraw(CTYPEDRAW_FUNC_ARGS)
{
	if (sim->elements[t].Properties & TYPE_SOLID)
		return false;
	return basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS);
}

void STOR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STOR";
	elem->Name = "STOR";
	elem->Colour = COLPACK(0x50DFDF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
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

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "储存体。捕获并保存一个粒子；用 PSCN 通电时释放，也可传给 PIPE。";
	elem->DetailedDescription = "描述：固体，吸收一个像素的与之接触的物质，修改它的Ctype 值可以使它吸收特定的物质，用 P 型硅(PSCN)或\nA 射线发射器(ARAY)的射线激活后释放，也可以传递给动力管(PIPE)。\n91.0beta 后，堆栈也可以将物质传递给传送门入口(PRTI)。\n元素参数：\nTmp=被吸收元素的 type 值\nTemp=被吸收元素的 Temp 值\nTmp2=被吸收元素的 Life 值\nPavg[0]=被吸收元素的 Tmp 值\nPavg[1]=被吸收元素的 Ctype 值\n导热率：0\n初始温度：22.00℃/295.15K";

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

	elem->Update = &STOR_update;
	elem->Graphics = &STOR_graphics;
	elem->CtypeDraw = &STOR_ctypeDraw;
	elem->Init = &STOR_init_element;
}
