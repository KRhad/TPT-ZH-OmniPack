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

void GAS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GAS";
	elem->Name = "GAS";
	elem->Colour = COLPACK(0xE0FF20);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.75f;
	elem->HotAir = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 600;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP + 2.0f + 273.15f;
	elem->HeatConduct = 42;
	elem->Latent = 0;
	elem->Description = "迅速扩散且易燃。在压力下液化成OIL。";
	elem->DetailedDescription = "描述：易燃气体,当其温度低于 60 摄氏度和处于或高于+6 P 压力时，GAS 将转变回 OIL。PTNM 在 2 P 和 200℃时接触 GAS 会使其变成 INSL。\n燃点：299.85℃/573K\n液化压力：6 P\n产生：中子轰击石油(OIL)或柴油(DESL)。在低压/加热下石油会变成石油气。\n导热率：42\n初始温度：24.00℃/297.15K";

	elem->Properties = TYPE_GAS|PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 6.0f;
	elem->HighPressureTransitionElement = PT_OIL;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 573.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &GAS_init_element;
}
