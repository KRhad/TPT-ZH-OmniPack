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

int BREL_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life)
	{
		if (sim->air->pv[y/CELL][x/CELL] > 10.0f)
		{
			if (parts[i].temp>9000 && (sim->air->pv[y/CELL][x/CELL] > 30.0f) && RNG::Ref().chance(1, 200))
			{
				part_change_type(i, x, y, PT_EXOT);
				parts[i].life = 1000;
			}
			parts[i].temp += (sim->air->pv[y/CELL][x/CELL])/8;
		}
	}
	return 0;
}

void BREL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BREC";
	elem->Name = "BREL";
	elem->Colour = COLPACK(0x707060);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.18f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 2;
	elem->Hardness = 2;

	elem->Weight = 90;

	elem->HeatConduct = 211;
	elem->Latent = 0;
	elem->Description = "损坏的电子元件。由 EMP 爆炸产生；受压并持续通电时会变为 EXOT。";
	elem->DetailedDescription = "描述：使用电磁脉冲武器(EMP)摧毁电子设备留下的物质，不能重铸，能导电。振金(VIBR)爆炸时会产生 BREL。在 10 P 以上压力下通电会不断升温，在 30 P 以上压力下通电会形成奇异物质(EXOT)，在 1000℃ 和高于 50P的压力下此反应可以被铂催化。在240℃以上与 BRMT结合时，它们会形成THRM 和 BRMT/BREL。\n导热率：211\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BREL_update;
	elem->Graphics = NULL;
	elem->Init = &BREL_init_element;
}
