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

int WATR_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_SALT && RNG::Ref().chance(1, 50))
				{
					part_change_type(i,x,y,PT_SLTW);
					// on average, convert 3 WATR to SLTW before SALT turns into SLTW
					if (RNG::Ref().chance(1, 3))
						part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
				}
				else if ((TYP(r)==PT_RBDM||TYP(r)==PT_LRBD) && (legacy_enable||parts[i].temp>(273.15f+12.0f)) && RNG::Ref().chance(1, 100))
				{
					part_change_type(i,x,y,PT_FIRE);
					parts[i].life = 4;
					parts[i].ctype = PT_WATR;
				}
				else if (TYP(r)==PT_FIRE && parts[ID(r)].ctype!=PT_WATR)
				{
					sim->part_kill(ID(r));
					if (RNG::Ref().chance(1, 30))
					{
						sim->part_kill(i);
						return 1;
					}
				}
				else if (TYP(r)==PT_SLTW && RNG::Ref().chance(1, 2000))
				{
					part_change_type(i,x,y,PT_SLTW);
				}
				else if (TYP(r) == PT_ROCK && fabs(parts[i].vx) + fabs(parts[i].vy) >= 0.5 && RNG::Ref().chance(1, 1000)) // ROCK erosion
				{
					if (RNG::Ref().chance(1,3))
						sim->part_change_type(ID(r), x + rx, y + ry, PT_SAND);
					else
						sim->part_change_type(ID(r), x + rx, y + ry, PT_STNE);
				}
				/*if (TYP(r)==PT_CNCT && RNG::Ref().chance(1, 100))	Concrete+Water to paste, not very popular
				{
					part_change_type(i,x,y,PT_PSTE);
					kill_part(ID(r));
				}*/
			}
	return 0;
}

void WATR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WATR";
	elem->Name = "WATR";
	elem->Colour = COLPACK(0x2030D0);
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

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 7500;
	elem->Description = "水。导电，可结冰，并能灭火。";
	elem->DetailedDescription = "描述：普通水(WATR)可以导电。蒸馏水(DSTW)接触多数杂质后会变成 WATR；植物(PLNT)能吸收它生长。NEUT 穿过水时会逐步把 WATR 转成 DSTW，同时自身减速并可能被吸收。\n沸点：99.85℃/373K\n凝固点：0℃/273.15K\n相变：温度达到 99.86℃+2×压力时变成 WTRV 并增加约 0.5 P；温度不高于 -0.01℃时，压力至少 0.8 P 生成 SNOW，否则生成 Ctype=WATR 的 ICE。\n产生：BOYL+OXYG→WATR；低压 HYGN+DESL→WATR+OIL；BUBW 放置一段时间后→WATR+CO2；SPNG 可释放已吸收的 WATR；RIME 在 0℃以上→WATR。\n电与爆炸：SPRK 可在 WATR 中缓慢传导。WATR 可熄灭普通 FIRE；接触 LRBD/RBDM 时生成 WTRV 和高温 FIRE。\n与气体和液体：CO2→BUBW；BOYL→FOG；DSTW 会被污染成 WATR；与 SLTW 混合得到更多 SLTW；与 GLOW 逐步生成 DEUT；与 FRZW 接触会转为 FRZW；GEL、SPNG 可吸水并增加相应储水参数。\n与粉末和固体：SALT→SLTW；FRZZ→FRZW；CLST→PSTE；PLNT 吸收后生长；IRON 被水腐蚀为 BMTL。\n与辐射：NEUT 使 WATR→DSTW 并减速；ELEC 作用后生成 HYGN 和 OXYG，同时电子被反射。\n导热率：29\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_LIQUID | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 273.15f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &WATR_update;
	elem->Graphics = NULL;
	elem->Init = &WATR_init_element;
}
