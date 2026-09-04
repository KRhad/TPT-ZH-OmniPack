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

unsigned int wavelengthToDecoColour(int wavelength)
{
	int colr = 0, colg = 0, colb = 0, x;
	for (x=0; x<12; x++) {
		colr += (wavelength >> (x+18)) & 1;
		colb += (wavelength >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		colg += (wavelength >> (x+9))  & 1;
	x = 624/(colr+colg+colb+1);
	colr *= x;
	colg *= x;
	colb *= x;

	if(colr > 255) colr = 255;
	else if(colr < 0) colr = 0;
	if(colg > 255) colg = 255;
	else if(colg < 0) colg = 0;
	if(colb > 255) colb = 255;
	else if(colb < 0) colb = 0;

	return (255<<24) | (colr<<16) | (colg<<8) | colb;
}

int CRAY_update(UPDATE_FUNC_ARGS)
{
	// set ctype to things that touch it if it doesn't have one already
	if (parts[i].ctype<=0 || !sim->elements[TYP(parts[i].ctype)].Enabled)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
			{
				int r = photons[y+ry][x+rx];
				if (!r)
					r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) != PT_CRAY && TYP(r) != PT_PSCN && TYP(r) != PT_INST && TYP(r) != PT_METL && TYP(r) != PT_SPRK)
				{
					parts[i].ctype = TYP(r);
					parts[i].temp = parts[ID(r)].temp;
				}
			}
	}
	else
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!TYP(r))
						continue;
					// Spark found, start creating
					if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3)
					{
						ARGBColour colored = COLARGB(0, 0, 0, 0);
						int destroy = parts[ID(r)].ctype == PT_PSCN;
						int nostop = parts[ID(r)].ctype == PT_INST;
						int createSpark = parts[ID(r)].ctype == PT_INWR;
						int partsRemaining = 255;
						// How far it shoots
						if (parts[i].tmp)
							partsRemaining = parts[i].tmp;
						int spacesRemaining = parts[i].tmp2;

						for (int docontinue = 1, nxi = rx*-1, nyi = ry*-1, nxx = spacesRemaining*nxi, nyy = spacesRemaining*nyi; docontinue; nyy+=nyi, nxx+=nxi)
						{
							if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0))
								break;

							r = pmap[y+nyi+nyy][x+nxi+nxx];
							// Create, also set color if it has passed through FILT
							if (!sim->IsWallBlocking(x+nxi+nxx, y+nyi+nyy, TYP(parts[i].ctype)) && (!pmap[y+nyi+nyy][x+nxi+nxx] || createSpark))
							{
								int nr = sim->part_create(-1, x+nxi+nxx, y+nyi+nyy, TYP(parts[i].ctype), ID(parts[i].ctype));
								if (nr!=-1)
								{
									if (colored)
										parts[nr].dcolour = colored;
									parts[nr].temp = parts[i].temp;
									if (parts[i].life > 0)
										parts[nr].life = parts[i].life;
									if (!--partsRemaining)
										docontinue = 0;
								}
							// Get color if passed through FILT
							}
							else if (TYP(r)==PT_FILT)
							{
								if (parts[ID(r)].dcolour == COLRGB(0, 0, 0))
									colored = COLRGB(0, 0, 0);
								else if (parts[ID(r)].tmp == 0)
									colored = wavelengthToDecoColour(getWavelengths(&parts[ID(r)]));
								else if (colored == COLRGB(0, 0, 0))
									colored = COLARGB(0, 0, 0, 0);
								parts[ID(r)].life = 4;
							}
							else if (TYP(r) == PT_CRAY || nostop)
							{
								docontinue = 1;
							}
							else if (destroy && r && TYP(r) != PT_DMND)
							{
								sim->part_kill(ID(r));
								if (!--partsRemaining)
									docontinue = 0;
							}
							else
								docontinue = 0;
							if (!partsRemaining)
								docontinue = 0;
						}
					}
				}
	}
	return 0;
}

bool CRAY_ctypeDraw(CTYPEDRAW_FUNC_ARGS)
{
	if (!ctypeDrawVInCtype(CTYPEDRAW_FUNC_SUBCALL_ARGS))
		return false;
	if (t == PT_LIGH)
		sim->parts[i].ctype |= PMAPID(30);
	sim->parts[i].temp = sim->elements[t].DefaultProperties.temp;
	return true;
}

void CRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CRAY";
	elem->Name = "CRAY";
	elem->Colour = COLPACK(0xBBFF00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
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
	elem->Description = "粒子射线发射器。创建由其 ctype 设置的粒子束，其范围由 tmp 设置。";
	elem->DetailedDescription = "描述：当 CRAY 从与粒子直接相邻(接触)的任何一侧(包括对角线)发出 SPRK 时，它将向相反方向发射粒子束。其属性取决于引发它的导体、温度、Tmp、Tmp2、Ctype 和 Life 的组合。Ctype 设置 CRAY 产生的粒子的类型。Ctype 为 0(默认)意味着它将其 Ctype 设置为接触它的第一个粒子的类型，包括导体。Temp 设置产生的粒子的温度。Life 设置产生的粒子的寿命。Tmp 设置射线的长度。当 CRAY 激活时，产生 Tmp 值长的射线。Tmp2 设置产生的射线与发射端之间间隔的距离。\n性质：\n被 NSCN 所激活的 CRAY 的性质有\n1.射线(即创造元素)可以穿过滤镜，但不能穿过其他元素。被 INST 所激活的 CRAY 性质有\n1.射线不仅能穿过滤镜还能穿过其他元素。被 INWR 所激活的 CRAY 性质有\n1.射线不仅能穿过滤镜还能穿过其他元素。\n2.能发射 SPRK 使在其直线 Tmp2 格后的其 Tmp 值个的导体通电。被 PSCN 所激活的 CRAY 特性有\n1.删除在在其直线上的 Tmp2 格后的其(Tmp2+Tmp)格前的元素，并将原来是空缺的格子用编号为其Ctype 的元素填充。\n2.不能删除 FILT，DMND。\n3.会被 DMND 所阻挡。被 PSCN 所激活的 CRAY(Ctype 为 SPRK)特性有\n1.删除在在其直线上的 Tmp2 格后的其 Tmp 值个元素。\n2.不能删除 FILT，DMND。\n3.会被 DMND 所阻挡。\n导热率：0\n初始温度：22℃/295.15K";

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

	elem->Update = &CRAY_update;
	elem->Graphics = NULL;
	elem->CtypeDraw = &CRAY_ctypeDraw;
	elem->Init = &CRAY_init_element;
}
