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

void PSCN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PSCN";
	elem->Name = "PSCN";
	elem->Colour = COLPACK(0x805050);
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
	elem->Meltable = 1;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x00000000;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "P 型硅。可向任何导体传导电流；可开启可控材料。";
	elem->DetailedDescription = "描述：可以电脉冲传导给任何导体，可以熔化。P 型硅与 N 型硅(NSCN)紧贴可以形成一个简单的太阳能电池板，有光子撞击 P 型硅的时候会产生电脉冲。另外，P 型硅也可以用来激活一些动力材料和制作二极管。1 个像素厚的 P 型硅层，铺在 N 型硅表面，可制成简易的太阳能电池板。通常用于激活动力材料或二极管。\n熔点：1413.85℃/1687.15K，变成 LAVA(PSCN)\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1687.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &PSCN_init_element;
}
