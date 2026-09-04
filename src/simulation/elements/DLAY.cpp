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

int DLAY_update(UPDATE_FUNC_ARGS)
{
	int oldl = parts[i].life;
	if (parts[i].life>0)
		parts[i].life--;
	
	if (parts[i].temp<= 1.0f+273.15f)
		parts[i].temp = 1.0f+273.15f;

	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int pavg = parts_avg(i, ID(r), PT_INSL);
				if (pavg == PT_INSL || pavg == PT_RSSS)
					continue;
				if (TYP(r)==PT_SPRK && parts[i].life==0 && parts[ID(r)].life>0 && parts[ID(r)].life<4 && parts[ID(r)].ctype==PT_PSCN)
				{
					parts[i].life = (int)(parts[i].temp-273.15f+0.5f);
				}
				else if (TYP(r)==PT_DLAY)
				{
					if (!parts[i].life)
					{
						if (parts[ID(r)].life)
						{
							parts[i].life = parts[ID(r)].life;
							if ((ID(r))>i) //If the other particle hasn't been life updated
								parts[i].life--;
						}
					}
					else if (!parts[ID(r)].life)
					{
						parts[ID(r)].life = parts[i].life;
						if ((ID(r))>i) //If the other particle hasn't been life updated
							parts[ID(r)].life++;
					}
				}
				else if(TYP(r)==PT_NSCN && oldl==1)
				{
					sim->spark_conductive_attempt(ID(r), x+rx, y+ry);
				}
			}
	return 0;
}

int DLAY_graphics(GRAPHICS_FUNC_ARGS)
{
	int stage = (int)(((float)cpart->life/(cpart->temp-273.15))*100.0f);
	*colr += stage;
	*colg += stage;
	*colb += stage;
	return 0;
}

void DLAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DLAY";
	elem->Name = "DLAY";
	elem->Colour = COLPACK(0x753590);
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
	elem->Meltable = 1;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 4.0f + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "延迟导体，延迟时间随温度变化（使用 HEAT/COOL 调节）。";
	elem->DetailedDescription = "描述：当电脉冲通过延时计时会延迟 X 帧，X 等于延时计的温度，不导热，可以使用升温笔(HEAT)和降温笔(COOL)来改变温度，最低为 1℃。DLAY 可以在大量使用时正常工作，但是 PSCN 和 NSCN 之间必须有两个像素的间隙，否则电脉冲会跳过 1 个像素的间隙，并且会出现两个电脉冲，这在某些情况下很有用。\n过程描述：\n电脉冲输入(PSCN) Life 值变为当前温度，颜色变亮每过一帧，Life–1，直到 Life=0，颜色变暗电脉冲输出(NSCN)\n导热率：0\n初始温度：4.00℃/277.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DLAY_update;
	elem->Graphics = &DLAY_graphics;
	elem->Init = &DLAY_init_element;
}
