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

int URAN_update(UPDATE_FUNC_ARGS)
{
	if (!legacy_enable && sim->air->pv[y/CELL][x/CELL]>0.0f)
	{
		if (parts[i].temp == MIN_TEMP)
		{
			parts[i].temp += .01f;
		}
		else
		{
			parts[i].temp = restrict_flt((parts[i].temp*(1 + (sim->air->pv[y / CELL][x / CELL] / 2000))) + MIN_TEMP, MIN_TEMP, MAX_TEMP);
		}
	}
	return 0;
}

void URAN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_URAN";
	elem->Name = "URAN";
	elem->Colour = COLPACK(0x707020);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.4f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;
	elem->PhotonReflectWavelengths = 0x003FC000;

	elem->Weight = 90;

	elem->DefaultProperties.temp = R_TEMP + 30.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "铀。重颗粒。在压力下产生热量。";
	elem->DetailedDescription = "描述：核反应的副产品，在压力下会快速上升温度(指数关系)，在低压或没有压力时会缓慢冷却。现实中的核反应堆利用铀产生热量和蒸汽，蒸汽用于旋转涡轮机并凝结成水，涡轮机的速度在发电机中提供动力。由于产生蒸汽压力的反应增加了加热。压力引起的热量变化率以指数方式确定，256 P 的压力下温度每帧增加 83.01℃。\n导热率：251\n初始温度：52.00℃/325.15K";

	elem->Properties = TYPE_PART | PROP_RADIOACTIVE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &URAN_update;
	elem->Graphics = NULL;
	elem->Init = &URAN_init_element;
}
