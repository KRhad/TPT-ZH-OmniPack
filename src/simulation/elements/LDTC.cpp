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
#include "simulation/elements/FILT.h"

const int FLAG_INVERT_FILTER =  0x1;
const int FLAG_IGNORE_ENERGY =  0x2;
const int FLAG_NO_COPY_COLOR =  0x4;
const int FLAG_KEEP_SEARCHING = 0x8;

//NOTES:
// ctype is used to store the target element, if any. (NONE is treated as a wildcard)
// life is used for the amount of pixels to skip before starting the scan. Starts just in front of the LDTC if 0.
// tmp is the number of particles that will be scanned before scanning stops. Unbounded if 0.
// tmp2 is used for settings (binary flags). The flags are as follows:
// 0x01: Inverts the CTYPE filter so that the element in ctype is the only thing that doesn't trigger LDTC, instead of the opposite.
// 0x02: Ignore energy particles
// 0x04: Ignore FILT (do not use color copying mode)
// 0x08: Keep searching even after finding a particle

/* Returns true for particles that activate the special FILT color copying mode */
bool phot_data_type(int rt)
{
	return rt == PT_FILT || rt == PT_PHOT || rt == PT_BRAY || rt == PT_BIZR || rt == PT_BIZRG || rt == PT_BIZRS;
}

/* Returns true for particles that start a ray search ("dtec" mode) */
bool accepted_conductor(Simulation* sim, int r)
{
	int rt = TYP(r);
	return (sim->elements[rt].Properties & PROP_CONDUCTS) && !(rt == PT_WATR || rt == PT_SLTW || rt == PT_NTCT ||
	        rt == PT_PTCT || rt == PT_INWR) && sim->parts[ID(r)].life == 0;
}

int LDTC_update(UPDATE_FUNC_ARGS)
{
	int ctype = TYP(parts[i].ctype), ctypeExtra = ID(parts[i].ctype), detectLength = parts[i].tmp, detectSpaces = parts[i].tmp2;
	bool copyColor = !(parts[i].tmp2 & FLAG_NO_COPY_COLOR);
	bool ignoreEnergy = parts[i].tmp2 & FLAG_IGNORE_ENERGY;
	bool invertFilter = parts[i].tmp2 & FLAG_INVERT_FILTER;
	bool keepSearching = parts[i].tmp2 & FLAG_KEEP_SEARCHING;
	if (detectSpaces < 0)
		detectSpaces = parts[i].tmp2 = 0;
	if (detectLength < 0)
		detectLength = parts[i].tmp = 0;
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				bool boolMode = accepted_conductor(sim, r);
				bool filtMode = copyColor && TYP(r) == PT_FILT;
				if (!boolMode && !filtMode)
					continue;

				int maxRange = parts[i].life + parts[i].tmp;
				int xStep = rx * -1, yStep = ry * -1;
				int xCurrent = x + (xStep * (parts[i].life + 1)), yCurrent = y + (yStep * (parts[i].life + 1));
				for (; !parts[i].tmp ||
					(xStep * (xCurrent - x) <= maxRange &&
					yStep * (yCurrent - y) <= maxRange);
					xCurrent += xStep, yCurrent += yStep)
				{
					if (!(xCurrent>=0 && yCurrent>=0 && xCurrent<XRES && yCurrent<YRES))
						break; // We're out of bounds! Oops!
					int rr = pmap[yCurrent][xCurrent];
					if (!rr && !ignoreEnergy)
						rr = photons[yCurrent][xCurrent];
					if (!rr)
						continue;

					// If ctype isn't set (no type restriction), or ctype matches what we found
					// Can use .tmp2 flag to invert this
					bool matchesCtype = ctype == TYP(rr) && (ctype != PT_LIFE || parts[ID(rr)].ctype == ctypeExtra);
					bool matchesFilter = !ctype || (invertFilter ^ (int)matchesCtype);
					if (!matchesFilter)
					{
						if (keepSearching)
							continue;
						else
							break;
					}
					// room for more conditions here.

					if (boolMode)
					{
						parts[ID(r)].life = 4;
						parts[ID(r)].ctype = TYP(r);
						sim->part_change_type(ID(r), x + rx, y + ry, PT_SPRK);
						break;
					}

					if (filtMode)
					{
						if (!phot_data_type(TYP(rr)))
							continue;

						int nx = x + rx, ny = y + ry;
						int photonWl = TYP(rr) == PT_FILT ?
							getWavelengths(&parts[ID(rr)]) :
							parts[ID(rr)].ctype;
						while (TYP(r) == PT_FILT)
						{
							parts[ID(r)].ctype = photonWl;
							nx += rx;
							ny += ry;
							if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
								break;
							r = pmap[ny][nx];
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}

void LDTC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LDTC";
	elem->Name = "LDTC";
	elem->Colour = COLPACK(0x66ff66);
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
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "线性探测器。在 8 个方向上扫描具有 ctype 的粒子，并在相反的一侧产生火花。";
	elem->DetailedDescription = "描述：从 8 个方向(上、下、左、右、四个 45 度对角线)检测粒子，并在相反方向激发导体。它检测到的粒子由其 Ctype 值设置。如果未设置 Ctype 值，它可以检测任何粒子。请注意，粒子必须与要检测的导体和 LDTC 成一直线(因此得名线性检测器)。LDTC 的 Tmp 值决定了它的范围(它可以检测到多远的粒子)。范围并不局限于 25。LDTC 的 Life 值决定了它在扫描前会跳过多少个像素。例如，如果它的 Life 值是 10，它的 Tmp 是 15，它会跳过 10 个像素，开始扫描 15 个像素，然后停止。跟 DTEC 一样可以把 FILT 和 BRAY 的 Ctype 值，复制到相反方向的 FILT 上。\n注意：一旦找到粒子，LDTC 将停止扫描，无论该粒子是否与其 Ctype 匹配。除非它的 Life 值设置为>1，否则在它周围放置导体会妨碍它的功能。这可以通过改变元素参数来改变，如下所示。\n元素参数：\nTmp 设定检测的范围：\nTmp 2=1 反转模式(所有不是 Ctype 值的元素会激发 LDTC)\nTmp 2=2 忽略能量粒子\nTmp 2=4 忽略颜色(不区分颜色)\nTmp 2=8 扫描粒子后方的物体(如果没有设定该值,LDTC 将不会检测隐藏在粒子后方的粒子)\nLife：会跳过多少像素，才开始扫描\n导热率：0\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NOCTYPEDRAW;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &LDTC_update;
	elem->Graphics = nullptr;
	elem->CtypeDraw = &ctypeDrawVInCtype;
	elem->Init = &LDTC_init_element;
}
