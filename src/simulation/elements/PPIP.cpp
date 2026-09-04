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
#include "PPIP.h"

int PIPE_update(UPDATE_FUNC_ARGS);
int PIPE_graphics(GRAPHICS_FUNC_ARGS);

void PPIP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PPIP";
	elem->Name = "PPIP";
	elem->Colour = COLPACK(0x444466);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
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

	elem->DefaultProperties.temp = 295.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "通电控制的管道。";
	elem->DetailedDescription = "描述：动力管(PIPE)的可控形式，利用 P 型硅(PSCN)激活时其中的物质将会运输，用 N 型硅(NSCN)则会停止其中物质的运输，用超导线(INST)会使物质向反方向运输。当激活时，周围包裹的砖块(BRCK)会发出蓝光。其他使用方法请参考动力管(PIPE)。PPIP 是少数使用 Pavg0 和 Pavg1 的元素之一。Pavg1 用于存储 BIZR/S/G 或 PHOT的颜色\n导热率：0\n初始温度：0.00℃/273.15K";

	elem->Properties = TYPE_SOLID | PROP_LIFE_DEC;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 60;

	elem->Update = &PIPE_update;
	elem->Graphics = &PIPE_graphics;
	elem->Init = &PPIP_init_element;

	sim->elementData[t].reset(new PPIP_ElementDataContainer);
}

void PPIP_ElementDataContainer::Simulation_BeforeUpdate(Simulation *sim)
{
	if (ppip_changed)
	{
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
		{
			if (parts[i].type == PT_PPIP)
			{
				parts[i].tmp |= (parts[i].tmp&0xE0000000)>>3;
				parts[i].tmp &= ~0xE0000000;
			}
		}
		ppip_changed = false;
	}
}
