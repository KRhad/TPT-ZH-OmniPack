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

int PINV_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life >= 10)
	{
		*cola = 100;
		*colr = 15;
		*colg = 0;
		*colb = 150;
		*pixel_mode &= PMODE;
		*pixel_mode |= PMODE_BLEND;
	} 
	return 0;
}

void PINV_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PINV";
	elem->Name = "PINV";
	elem->Colour = COLPACK(0x00CCCC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 15;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "可控隐形材料，激活时对粒子不可见。";
	elem->DetailedDescription = "描述：可控隐形材料。PSCN 开启整片 PINV，NSCN 关闭；开启状态(Life≥10)允许普通粒子进入并穿过，关闭时成为不可破坏的固体屏障。开启时颜色变为半透明紫色。\n穿透：中子和光子可穿过 PINV。普通粒子穿过时可以与 PINV 占据同一像素，内部粒子由专用槽保存；关闭后不再允许新的普通粒子进入。\n特性：不导热，不会因普通压力或温度发生相变。\n导热率：0\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_PHOTPASS | PROP_POWERED | PROP_INDESTRUCTIBLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &PINV_graphics;
	elem->Init = &PINV_init_element;
}
