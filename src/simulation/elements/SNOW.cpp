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

int ICE_update(UPDATE_FUNC_ARGS);

void SNOW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SNOW";
	elem->Name = "SNOW";
	elem->Colour = COLPACK(0xC0E0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.90f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.05f;
	elem->Diffusion = 0.01f;
	elem->HotAir = -0.00005f* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;
	elem->PhotonReflectWavelengths = 0x03FFFFFF;

	elem->Weight = 50;

	elem->DefaultProperties.temp = R_TEMP - 30.0f + 273.15f;
	elem->HeatConduct = 46;
	elem->Latent = 1095;
	elem->Description = "雪。ICE 受压破碎时形成的轻质粒子。";
	elem->DetailedDescription = "描述：轻粉末，冰(ICE)在压力下破坏形成雪，加热后变成水(WATR)。可以使中子(NEUT)减速。\n熔点：-0.15℃/273K\n产生：可以压力下通过冷却 WATR、DSTW、BUBW 或 SLTW 来产生雪，对 ICE 施加压力也可产生雪。\n反应：雪会在 0℃ 融化成它 Ctype 值的元素 (即使该元素不是一种水，可以用此特性来制作奇点炸弹)，除了SLTW，它会在-21.1℃ 融化。\n导热率：46\n初始温度：-8.00℃/265.15K";

	elem->Properties = TYPE_PART|PROP_NEUTPASS;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 252.05f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &ICE_update;
	elem->Graphics = NULL;
	elem->Init = &SNOW_init_element;
}
