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

void DUST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DUST";
	elem->Name = "DUST";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 10;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;
	elem->PhotonReflectWavelengths = 0x3FFFFFC0;

	elem->Weight = 85;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "非常轻的灰尘。易燃。";
	elem->DetailedDescription = "描述：轻粉末，难燃烧且火焰微弱。火柴人(STKM)一开始就能产生尘埃。用 NEUT 轰击时，DUST 会变 FWRK。燃烧时，它会产生 FIRE，Life 值在 185 到 255 之间时变成 SMKE。DUST 是启动 TPT 时默认选择的粒子。这是因为它的元素 ID 为 1。默认情况下，火柴人也会默认发射 DUST。\n产生：可以通过加热 DYST 或者用 NEUT 轰击 GUN 产生。\n元素参数：点燃后默认燃烧 10 帧，修改 Life 值可以改变其燃烧时间\n导热率：70\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &DUST_init_element;
}
