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

int DTEC_update(UPDATE_FUNC_ARGS)
{
	int rd = parts[i].tmp2;
	if (rd > 25) parts[i].tmp2 = rd = 25;
	if (parts[i].life)
	{
		parts[i].life = 0;
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					int rt = TYP(r);
					int pavg = parts_avg(i, ID(r), PT_INSL);
					if (pavg != PT_INSL && pavg != PT_RSSS)
					{
						if ((sim->elements[rt].Properties&PROP_CONDUCTS) && !(rt==PT_WATR||rt==PT_SLTW||rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR) && parts[ID(r)].life==0)
						{
							sim->spark_conductive(ID(r), x+rx, y+ry);
						}
					}
				}
	}
	bool setFilt = false;
	int photonWl = 0;
	for (int rx = -rd; rx < rd + 1; rx++)
		for (int ry = -rd; ry < rd + 1; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					r = photons[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == parts[i].ctype && (parts[i].ctype != PT_LIFE || parts[i].tmp == parts[ID(r)].ctype || !parts[i].tmp))
					parts[i].life = 1;
				if (TYP(r) == PT_PHOT || (TYP(r) == PT_BRAY && parts[ID(r)].tmp!=2) || TYP(r) == PT_BIZR || TYP(r) == PT_BIZRG || TYP(r) == PT_BIZRS)
				{
					setFilt = true;
					photonWl = parts[ID(r)].ctype;
				}
			}
	if (setFilt)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					int nx = x+rx;
					int ny = y+ry;
					while (TYP(r)==PT_FILT)
					{
						parts[ID(r)].ctype = photonWl;
						nx += rx;
						ny += ry;
						if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
							break;
						r = pmap[ny][nx];
					}
				}
	}
	return 0;
}

int DTEC_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life)
		*pixel_mode |=  PMODE_GLOW;
	return 0;
}

void DTEC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DTEC";
	elem->Name = "DTEC";
	elem->Colour = COLPACK(0xFD9D18);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SENSOR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "探测器，当具有其 ctype 的物体在附近时会产生火花。";
	elem->DetailedDescription = "描述：和使用复制体(CLNE)的方法基本一致，放置好探测器后，将需要探测的物质与之直接接触就能设置它的Ctype 值为这个物质的 Type 值，之后每当有相同的物质与之接触时都能产生一个电脉冲，可以由金属或导电体输出(导电墙不行)。是探测墙的缩小化替代品。DTEC 还可以在其 Tmp2 范围内检测 BRAY 和 PHOT 的 Ctype 并将其传输到相邻的 FILT。\n元素参数：Tmp2=侦测范围，最大 25 像素\n导热率：0\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp2 = 2;

	elem->Update = &DTEC_update;
	elem->Graphics = &DTEC_graphics;
	elem->CtypeDraw = &ctypeDrawVInTmp;
	elem->Init = &DTEC_init_element;
}
