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

int ACID_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);
				if (rt != PT_ACID && rt != PT_CAUS)
				{
					if (rt  == PT_PLEX || rt == PT_NITR || rt == PT_GUNP || rt == PT_RBDM || rt == PT_LRBD)
					{
						part_change_type(i, x, y, PT_FIRE);
						part_change_type(ID(r), x+rx, y+ry, PT_FIRE);
						parts[i].life = 4;
						parts[ID(r)].life = 4;
					}
					else if (rt == PT_WTRV)
					{
						if (RNG::Ref().chance(1, 250))
						{
							part_change_type(i, x, y, PT_CAUS);
							parts[i].life = RNG::Ref().between(25, 74);
							sim->part_kill(ID(r));
						}
					}
					else if (!(sim->elements[rt].Properties & PROP_CLONE) &&
							!(sim->elements[rt].Properties & PROP_INDESTRUCTIBLE) &&
							((rt != PT_FOG && rt != PT_RIME) || parts[ID(r)].tmp <= 5) &&
							parts[i].life > 50 && RNG::Ref().chance(sim->elements[rt].Hardness, 1000))
					{
						// GLAS protects stuff from acid
						if (parts_avg(i, ID(r), PT_GLAS) != PT_GLAS)
						{
							float newtemp = ((60.0f-(float)sim->elements[rt].Hardness))*7.0f;
							if (newtemp < 0)
								newtemp = 0;
							parts[i].temp += newtemp;
							parts[i].life--;
							switch (rt)
							{
								case PT_LITH:
									sim->part_change_type(ID(r), x + rx, y + ry, PT_H2);
									break;
									
								default:
									sim->part_kill(ID(r));
									break;
							}
						}
					}
					else if (parts[i].life <= 50)
					{
						sim->part_kill(i);
						return 1;
					}
				}
			}
	for (int trade = 0; trade < 2; trade++)
	{
		int rx = RNG::Ref().between(-2, 2);
		int ry = RNG::Ref().between(-2, 2);
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r) == PT_ACID && parts[i].life > parts[ID(r)].life && parts[i].life>0)//diffusion
			{
				int temp = parts[i].life - parts[ID(r)].life;
				if (temp == 1)
				{
					parts[ID(r)].life++;
					parts[i].life--;
				}
				else if (temp > 0)
				{
					parts[ID(r)].life += temp/2;
					parts[i].life -= temp/2;
				}
			}
		}
	}
	return 0;
}

int ACID_graphics(GRAPHICS_FUNC_ARGS)
{
	int s = cpart->life;
	if (s > 75)
		s = 75;
	else if (s < 49)
		s = 49;
	s = (s - 49) * 3;
	if (s == 0)
		s = 1;
	*colr += s * 4;
	*colg += s * 1;
	*colb += s * 2;
	*pixel_mode |= PMODE_BLUR;
	return 0;
}

void ACID_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ACID";
	elem->Name = "ACID";
	elem->Colour = COLPACK(0xED55FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 40;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;
	elem->PhotonReflectWavelengths = 0x1FE001FE;

	elem->Weight = 10;

	elem->HeatConduct = 34;
	elem->Latent = 0;
	elem->Description = "几乎溶解一切。";
	elem->DetailedDescription = "描述：可以腐蚀几乎所有物质，除了：岩浆(LAVA)、液氮(LN2)、放射性元素、特殊元素、爆炸物、玻璃(GLAS)、\n石英(QRTZ)、石英沙(PQRT)、钻石(DMND)、金(GOLD)、陶瓷(CRMC)等。可燃，可以由明火、电脉冲、岩浆点燃，\n生成酸气(CAUS)。材料的硬度值定义了 ACID 是否可以腐蚀材料、材料腐蚀的速度有多快。0 表示不能被 ACID 腐蚀。1 到1000 之间的硬度值表示可以被腐蚀，较小的值更容易发生反应。硬度值在 1 到 60 之间使反应放热，升温公式如下：温度_上升=(60-硬度)*7\n例：将水(WATR)粒子放在 ACID 粒子的顶部。水的硬度值为 20，这会导致酸加热 60-20=40*7=280 度。\n同位素 Z(ISOZ)：暴露于中子(NEUT)的酸会形成放射性元素同位素 Z。\n腐蚀性气体(CAUS)：与水蒸气(WTRV)接触的酸有 0.4%的几率反应成 CAUS。\n元素参数：每个酸粒子的 Life 值为 75。每进行一次反应，酸粒子的 Life 值就会减少 1。(模拟稀释)生命值达到 50 或以下时，酸性粒子将被消耗。每个酸粒子在被消耗之前都会腐蚀 25 个其他粒子。相互接触的酸性粒子会交换 Life值。(模拟扩散)酸对某些材料的处理方式不同，如果下面未列出，则预计该材料会被酸腐蚀。\n以下是特殊情况材料列表：\n爆炸物(将转化为两个 Life 值=4 的火(FIRE)粒子)\nC-4/塑料炸药(C-4/PLEX)\n硝酸甘油(NITR)\n火药(GUNP)\n铷(RBDM)\n液体铷(LRBD)\n水蒸气(WTRV)(最终所有的水蒸气都会变成腐蚀性气体，但这是一个缓慢的反应)\n玻璃(GLAS)(模拟实验室条件)*\n酸(ACID)和腐蚀性气体(CAUS)(出于显而易见的原因)复制体(CLNE)\n可控复制体(PCLN)\n黄金(GOLD)\n石英(QRTZ) (粉末形式也受到影响。)\n导热率：34\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_LIQUID|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 75;

	elem->Update = &ACID_update;
	elem->Graphics = &ACID_graphics;
	elem->Init = &ACID_init_element;
}
