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

int PSNS_update(UPDATE_FUNC_ARGS)
{
	if ((parts[i].tmp == 0 && sim->air->pv[y/CELL][x/CELL] > parts[i].temp - 273.15f) || (parts[i].tmp == 2 && sim->air->pv[y/CELL][x/CELL] < parts[i].temp - 273.15f))
	{
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
						if ((sim->elements[rt].Properties&PROP_CONDUCTS) && !(rt==PT_WATR || rt==PT_SLTW || rt==PT_NTCT || rt==PT_PTCT || rt==PT_INWR) && parts[ID(r)].life == 0)
						{
							sim->spark_conductive(ID(r), x+rx, y+ry);
						}
					}
				}
	}
	if (parts[i].tmp == 1) 
	{
		bool setFilt = true;
		float photonWl = sim->air->pv[y / CELL][x / CELL];
		if (setFilt)
		{
			for (int rx = -1; rx < 2; rx++)
				for (int ry = -1; ry < 2; ry++)
					if (rx || ry)
					{
						int r = pmap[y + ry][x + rx];
						if (!r)
							continue;
						int nx = x + rx;
						int ny = y + ry;
						while (TYP(r) == PT_FILT)
						{
							parts[ID(r)].ctype = 0x10000000 + roundl(photonWl) + 256;
							nx += rx;
							ny += ry;
							if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
								break;
							r = pmap[ny][nx];
						}
					}
		}
	}
	return 0;
}

void PSNS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PSNS";
	elem->Name = "PSNS";
	elem->Colour = COLPACK(0xDB2020);
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 4.0f + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "压力传感器，当压力大于其温度时会产生火花。";
	elem->DetailedDescription = "描述：不导热，可以用升温/降温笔(HEAT/COOL)来改变自身温度，当压力大于其温度值时，压力传感器会发出电脉冲给周围的导电体(水 WATR、盐水 SLTW、热敏电阻 NTCT&PTCT、绝缘线 INWR)。当 Tmp 设置为 1 时，它可以将其压力数据传输到 FILT(会占用第 29 位)。当 Tmp 被设定为 2，PSNS 将把电脉冲传给检测范围内的比它压力低的物质。\n导热率：0\n初始温度：4.00℃/277.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PSNS_update;
	elem->Graphics = NULL;
	elem->Init = &PSNS_init_element;
}
