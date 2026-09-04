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

void INWR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_INWR";
	elem->Name = "INWR";
	elem->Colour = COLPACK(0x544141);
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

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "绝缘导线。仅与 PSCN、NSCN、WIFI 和 SWCH 导电。";
	elem->DetailedDescription = "描述：只能在 P 型硅(PSCN)与 N 型硅(NSCN)之间传递电脉冲(双向)，可以熔化。INWR 比大多数其他元素具有更多受限制的 SPRK 传导规则，因此是“绝缘的”。INWR 执行 SPRK 往返的元素只有 5 个：\n双向传导：INWR、PSCN、NSCN。\n单向输出：INWR 可向 SWCH、WIFI 传导，但不会从它们接收。\n特点：INWR 对于 BRAY 射线是透明的，射线可以直接通过而不是被阻挡。这允许 BRAY 光束相互交叉—— BRAY光束通常会阻挡穿过它的其他白色 BRAY 光束，除非光束穿过的空间包含对 BRAY 透明的粒子。其他一些元素对 BRAY 也是透明的，例如 FILT 和 ARAY，但大多数都阻挡了 BRAY。INWR 不会被任何穿过它的 BRAY 光束激发(大多数导体在被 BRAY 击中时会被激发)。\n用途：可用于电子产品以允许为 SPRK 创建“交叉点”，因为 INWR 不会与大多数其他电子元件进行传导或传导。对 BRAY 的透明性意味着 INWR 可用作打印机中的 ROM(由 ARAY 读取)以存储图像和解码器。当反复通电时，INWR 会迅速冷却到 22℃，这在限制温度或需要快速冷却时非常有用。\n熔点：1413.85℃/1687K，变成 LAVA(INWR)\n导热率：251\n初始温度：22℃/295.15K";

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
	elem->Init = &INWR_init_element;
}
