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

#include "FILT.h"
#include "simulation/ElementsCommon.h"

int getWavelengths(particle* cpart)
{
	if (cpart->ctype&0x3FFFFFFF)
	{
		return cpart->ctype;
	}
	else
	{
		int temp_bin = (int)((cpart->temp-273.0f)*0.025f);
		if (temp_bin < 0) temp_bin = 0;
		if (temp_bin > 25) temp_bin = 25;
		return (0x1F << temp_bin);
	}
}

// Returns the wavelengths in a particle after FILT interacts with it (e.g. a photon)
// cpart is the FILT particle, origWl the original wavelengths in the interacting particle
int interactWavelengths(particle* cpart, int origWl)
{
	const int mask = 0x3FFFFFFF;
	int filtWl = getWavelengths(cpart);
	switch (cpart->tmp)
	{
	// Assign Color
	case 0:
		return filtWl;
	// Filter Color
	case 1:
		return origWl & filtWl;
	// Add Color
	case 2:
		return origWl | filtWl;
	case 3:
	// Subtract color of filt from color of photon
		return origWl & (~filtWl);
	// Red shift
	case 4:
	{
		int shift = int((cpart->temp-273.0f)*0.025f);
		if (shift<=0) shift = 1;
		return (origWl << shift) & mask;
	}
	// Blue shift
	case 5:
	{
		int shift = int((cpart->temp-273.0f)*0.025f);
		if (shift<=0) shift = 1;
		return (origWl >> shift) & mask;
	}
	case 6:
	// No change
		return origWl;
	case 7:
	// XOR colors
		return origWl ^ filtWl;
	case 8:
	// Invert colors
		return (~origWl) & mask;
	// "QTRZ scatter" mode
	case 9:
	{
		int t1 = (origWl & 0x0000FF) + RNG::Ref().between(-2, 2);
		int t2 = ((origWl & 0x00FF00)>>8) + RNG::Ref().between(-2, 2);
		int t3 = ((origWl & 0xFF0000)>>16) + RNG::Ref().between(-2, 2);
		return (origWl & 0xFF000000) | (t3<<16) | (t2<<8) | t1;
	}
	// Variable red shift
	case 10:
	{
		long long int lsb = filtWl & (-filtWl);
		return (origWl * lsb) & 0x3FFFFFFF;
	}
	// Variable blue shift
	case 11:
	{
		long long int lsb = filtWl & (-filtWl);
		return (origWl / lsb) & 0x3FFFFFFF;
	}
	default:
		return filtWl;
	}
}

int FILT_graphics(GRAPHICS_FUNC_ARGS)
{
	int x, wl = getWavelengths(cpart);
	*colg = 0;
	*colb = 0;
	*colr = 0;
	for (x=0; x<12; x++) {
		*colr += (wl >> (x+18)) & 1;
		*colb += (wl >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (wl >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	if (cpart->life>0 && cpart->life<=4)
		*cola = 127+cpart->life*30;
	else
		*cola = 127;
	*colr *= x;
	*colg *= x;
	*colb *= x;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_BLEND;
	return 0;
}

void FILT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = v;
}

void FILT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FILT";
	elem->Name = "FILT";
	elem->Colour = COLPACK(0x000056);
	elem->MenuVisible = 1;
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

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "滤镜。改变 PHOT 和 BIZR 的颜色，具体颜色取决于温度。";
	elem->DetailedDescription = "描述：滤镜(FILT)可改变光子(PHOT)和射线(BRAY)的波长与颜色。Ctype 用低 30 位保存波长数据；0x3FFFFFFF 或 -1 表示全部波长，显示为白色。Ctype=0 时由温度自动计算颜色：低温偏蓝，高温偏红，从 0℃起每升高约 40℃红移一位，1000℃附近达到红端。\n颜色结构：30 位分为红、黄、绿、青、蓝五组，长度依次为 9、3、6、3、9 位；显示颜色取决于各组中有效位的比例。FILT 不受环境热辐射影响，只与接触物交换热量。\n常见用途：FILT 导热率很高且不易损坏，可传递热量；与 ARAY 组合时可给 BRAY 着色、存储数据和执行逻辑运算。棕色 BRAY 或沿路径放置的透明粒子可清除旧射线，避免下一次 SPRK 周期受到干扰。\n操作模式(Tmp)：\n0 设置：把进入粒子的波长改为 FILT 的波长。\n1 过滤：原波长与 FILT 波长执行按位与。\n2 增加：执行按位或，加入 FILT 的波长。\n3 删除：从原波长中清除 FILT 对应的位。\n4 红移：按温度决定的位数向红端移动。\n5 蓝移：按温度决定的位数向蓝端移动。\n6 透明：不改变波长。\n7 异或：原波长与 FILT 波长执行按位异或。\n8 反色：30 位范围内按位取反；全白输入会被吸收。\n9 散射：模拟石英(QRTZ)的随机波长扰动。\n10 可变红移：按 FILT 最低有效位决定红移量。\n11 可变蓝移：按 FILT 最低有效位决定蓝移量。\n其他 Tmp 值：按模式 0 处理。\n逻辑说明：每一位都可独立参与运算，因此一个 FILT 像素可保存 30 位数据。若运算结果为 0，BRAY 会终止且不会输出 SPRK；实际电路常保留第 30 位作为存在标记，防止有效的零值被误判为没有数据。移位超出 30 位范围的数据会丢失。\n存储方式：简易存储是每个 FILT 保存一个值；参考存储用较小索引代替大数值；共享存储把多个短字段移入同一个 30 位值。DTEC 可把范围内 PHOT/BRAY 的 Ctype 写入相邻 FILT；LDTC 还能读取 PHOT、BRAY 或 FILT，并把值写到检测方向另一侧的 FILT。\n导热率：251\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_PHOTPASS | PROP_NOAMBHEAT | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &FILT_graphics;
	elem->Func_Create = &FILT_create;
	elem->Init = &FILT_init_element;
}
