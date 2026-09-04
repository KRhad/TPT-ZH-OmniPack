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

int VIRS_update(UPDATE_FUNC_ARGS);

int VRSS_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= NO_DECO;
	return 1;
}

void VRSS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VRSS";
	elem->Name = "VRSS";
	elem->Colour = COLPACK(0xD408CD);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SOLIDS;
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

	elem->DefaultProperties.temp = R_TEMP + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "固体病毒。将接触到的一切都变成病毒。";
	elem->DetailedDescription = "描述：89.0 版本后加入，会将其碰触到的所有物质变成病毒(VIRS)，一段时间后会自己死亡。肥皂(SOAP)可以治愈病毒(VIRS)并使物质恢复。质子(PROT)可以使病毒(VIRS)不会自动死亡。只能被等离子体(PLSM)点燃。不受 VIRS 影响的元素是能量类型元素引力子(GRVT)、PROT、电子(ELEC)、光子(PHOT)、中子(NEUT)、奇点\n(SING)、反物质(AMTR)和钻石 (DMND)。VIRS 是少数对 LOLZ 和 LOVE 产生有趣效果的元素之一。如果 VIRS 触及其中之一，则 VIRS 将被“克隆”，因为 VIRS 将 LOVE/LOLZ 的一个像素更改为更多 VIRS，但随后LOVE/LOLZ 会重组，将VIRS 推开。最终，VIRS 会同时腐蚀所有的 LOVE/LOLZ，否则 VIRS 将被完全推开并停止被克隆。\n沸点：399.85℃/673K 变成病毒气(VRSG)\n凝固点：31.85℃/305K 变成病毒块(VRSS)\n元素参数：Tmp2=感染物质的 Type 值\n导热率：251/251/251\n初始温度：72.00℃/345.15K";

	elem->Properties = TYPE_SOLID;
	elem->CarriesTypeIn = 1U << FIELD_TMP2;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 305.0f;
	elem->HighTemperatureTransitionElement = PT_VIRS;

	elem->DefaultProperties.tmp4 = 250;

	elem->Update = &VIRS_update;
	elem->Graphics = &VRSS_graphics;
	elem->Init = &VRSS_init_element;
}
