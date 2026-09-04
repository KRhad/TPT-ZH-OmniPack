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

int BRAY_graphics(GRAPHICS_FUNC_ARGS)
{
	int x, trans = 255;
	if (cpart->tmp == 0)
	{
		trans = cpart->life * 7;
		if (trans>255) trans = 255;
		if (cpart->ctype&0x3FFFFFFF)
		{
			*colg = 0;
			*colb = 0;
			*colr = 0;
			for (x=0; x<12; x++)\
			{
				*colr += (cpart->ctype >> (x+18)) & 1;
				*colb += (cpart->ctype >>  x)     & 1;
			}
			for (x=0; x<12; x++)
				*colg += (cpart->ctype >> (x+9))  & 1;
			x = 624/(*colr+*colg+*colb+1);
			*colr *= x;
			*colg *= x;
			*colb *= x;
		}
	}
	else if(cpart->tmp == 1)
	{
		trans = cpart->life/4;
		if (trans>255) trans = 255;
		if (cpart->ctype&0x3FFFFFFF)
		{
			*colg = 0;
			*colb = 0;
			*colr = 0;
			for (x=0; x<12; x++)
			{
				*colr += (cpart->ctype >> (x+18)) & 1;
				*colb += (cpart->ctype >>  x)     & 1;
			}
			for (x=0; x<12; x++)
				*colg += (cpart->ctype >> (x+9))  & 1;
			x = 624/(*colr+*colg+*colb+1);
			*colr *= x;
			*colg *= x;
			*colb *= x;
		}
	}
	else if (cpart->tmp == 2)
	{
		trans = cpart->life*100;
		if (trans>255) trans = 255;
		*colr = 255;
		*colg = 150;
		*colb = 50;
	}
	*cola = trans;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_BLEND | PMODE_GLOW;
	return 0;
}

void BRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BRAY";
	elem->Name = "BRAY";
	elem->Colour = COLPACK(0xFFFFFF);
	elem->MenuVisible = 0;
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
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "射线点。射线碰撞时会产生点。";
	elem->DetailedDescription = "导热率：0\n初始温度：22℃/295.15K\n其他：在 69 版本之后，ARAY 不再导热，其产生的 B 射线(BRAY)温度将会是 ARAY 的温度。这一特性被用来制作恒温器。\n染色：射线(BRAY)经过滤镜时会以设定好的染色方式染色。\n描述：可以从所有电导体，甚至 SWCH 接收 SPRK。能射出 B 射线(BRAY)，可以从任意导电物质中接受电脉冲，之后会沿着电脉冲的方向发射射线，多个射线相撞会产生固体B 射线(会慢慢消失)。与其他电子设备不同，ARAY必须从与其直接接触的像素接收 SPRK。\n其他模式：\n由 P 型硅(PSCN)输入电脉冲时会产生另一种不能导电的射线，会清除其他的 BRAY，并很快消失。\n由超导线(INST)输入电脉冲时产生的射线具有穿透性，可以穿透多个导电材料。来自 ARAY 的 BRAY 也可以具有不同的属性，具体取决于用于激发它的内容。如果设置了 ARAY 的 Life，则生成的 BRAY 将使用 ARAY 的 Life。BRAY 是 ARAY 被任何导体激活所产生的。它的颜色由 30 位色谱上的颜色之间的比率决定。它可以容纳 30位数字，并且默认情况下(除非它是由 PSCN 创建的)，所有 30 位都设置为白色。您可以使用 FILT 将该值设置为不同的值或对其执行按位运算。BRAY 元素在创建后迅速消失，Life 为 30 帧。与中子和光子不同，BRAY 穿过所有壁元素，除非碰到一个不透明的粒子。如果 BRAY 击中导体，例如 METL，则该元素会产生 SPRK。BRAY 也可以通过 ARAY。如果两条 BRAY 线发生碰撞，它们会在该点创建一个“实心”BRAY，该点将缓慢消失，Life 为 1020。“实心”BRAY的独特之处在于它也是透明的。因此，虽然 BRAY 通常不透明，但当它们创建“实体”BRAY时它会变得透明。任何穿过“固体”BRAY的白色 BRAY 都会将 BRAY 的 Life 恢复到 1020。用 PSCN 激活 ARAY 将创建一个棕色 BRAY。棕色 BRAY 类似于白色 BRAY，但不会激活导体，也不与 FILT 相互作用。它甚至没有波长。它还会擦除任何活动的白色 BRAY，在与自身碰撞时不会创建“实体”版本。透明元件BRAY 可以穿过的元件。透明元素包括“固体”BRAY、FILT、STOR、ARAY、INWR 和激活的 SWCH。INWR 比较特殊，BRAY 可以穿过 INWR 而且不会被激活。BRAY 与 SWCH BRAY 可以通过打开的 SWCH。如果 SWCH 关闭，则可以使用与其直接相邻的两个棕色 BRAY(与两条棕色BRAY 交点的位置)来打开它。如果 SWCH 处于开启状态，情况也是如此。";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 30;

	elem->Update = NULL;
	elem->Graphics = &BRAY_graphics;
	elem->Init = &BRAY_init_element;
}
