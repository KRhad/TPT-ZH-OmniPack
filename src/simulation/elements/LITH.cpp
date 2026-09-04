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

/*

tmp2:  carbonation factor
life:  burn timer above 1000, spark cooldown timer otherwise
tmp:   hydrogenation factor
ctype: absorbed energy

For game reasons, baseline LITH has the reactions of both its pure form and
its hydroxide, and also has basic li-ion battery-like behavior.
It absorbs CO2 like its hydroxide form, but can only be converted into GLAS
after having absorbed CO2.

*/

int LITH_update(UPDATE_FUNC_ARGS)
{
	particle &self = parts[i];

	int &hydrogenationFactor = self.tmp;
	int &burnTimer = self.life;
	int &carbonationFactor = self.tmp2;
	int &storedEnergy = self.ctype;
	if (storedEnergy < 0)
	{
		storedEnergy = 0;
	}

	bool charged = false;
	bool discharged = false;
	for (int rx = -2; rx <= 2; ++rx)
	{
		for (int ry = -2; ry <= 2; ++ry)
		{
			if (rx || ry)
			{
				int neighborData = pmap[y + ry][x + rx];
				if (!neighborData)
				{
					if (burnTimer > 1012 && RNG::Ref().chance(1, 10))
					{
						sim->part_create(-1, x + rx, y + ry, PT_FIRE);
					}
					continue;
				}
				particle &neighbor = parts[ID(neighborData)];
				
				switch (TYP(neighborData))
				{
					case PT_SLTW:
					case PT_WTRV:
					case PT_WATR:
					case PT_DSTW:
					case PT_CBNW:
						if (burnTimer > 1016)
						{
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_WTRV);
							neighbor.temp = 440.f;
							continue;
						}
						if (hydrogenationFactor + carbonationFactor >= 10)
						{
							continue;
						}
						if (self.temp > 440.f)
						{
							burnTimer = 1024 + (storedEnergy > 24 ? 96 : storedEnergy * 4);
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_H2);
							hydrogenationFactor = 10;
						}
						else
						{
							self.temp = restrict_flt(self.temp + 20.365f + storedEnergy * storedEnergy * 1.5f, MIN_TEMP, MAX_TEMP);
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_H2);
							hydrogenationFactor += 1;
						}
						break;
						
					case PT_CO2:
						if (hydrogenationFactor + carbonationFactor >= 10)
						{
							continue;
						}
						sim->part_kill(ID(neighborData));
						carbonationFactor += 1;
						break;
						
					case PT_SPRK:
					{
						int pavg = parts_avg(i, ID(neighborData), PT_INSL);
						if (pavg == PT_INSL || pavg == PT_RSSS)
						{
							break;
						}
						if (hydrogenationFactor + carbonationFactor >= 5)
						{
							continue; // too impure to do battery things.
						}
						if (neighbor.ctype == PT_PSCN && neighbor.life == 3 && !charged && !burnTimer)
						{
							charged = true;
						}
						break;
					}
						
					case PT_NSCN:
					{
						int pavg = parts_avg(i, ID(neighborData), PT_INSL);
						if (pavg == PT_INSL || pavg == PT_RSSS)
						{
							break;
						}
						if (neighbor.life == 0 && storedEnergy > 0 && !burnTimer)
						{
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_SPRK);
							neighbor.life = 4;
							neighbor.ctype = PT_NSCN;
							discharged = true;
						}
						break;
					}
						
					case PT_FIRE:
						if (self.temp > 440.f && RNG::Ref().chance(1, 40) && hydrogenationFactor < 6)
						{
							burnTimer = 1013;
							hydrogenationFactor += 1;
						}
						break;
						
					case PT_O2:
						if (burnTimer > 1000 && RNG::Ref().chance(1, 10))
						{
							sim->part_change_type(i, x, y, PT_PLSM);
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_PLSM);
							sim->air->pv[y / CELL][x / CELL] += 4.0;
							return 0;
						}						
						break;
				}
			}
		}
	}
	if (charged)
	{
		storedEnergy += 1;
		burnTimer = 8;
	}
	if (discharged)
	{
		storedEnergy -= 1;
		burnTimer = 8;
	}
	
	for (int trade = 0; trade < 9; ++trade)
	{
		int rx = RNG::Ref().between(-3, 3);
		int ry = RNG::Ref().between(-3, 3);
		if (rx || ry)
		{
			int neighborData = pmap[y + ry][x + rx];
			if (TYP(neighborData) != PT_LITH)
			{
				continue;
			}
			particle &neighbor = parts[ID(neighborData)];
			
			int &neighborStoredEnergy = neighbor.ctype;
			// Transfer overcharge explosion status to nearby LITH
			if (burnTimer < 1000 && storedEnergy > 90 && neighbor.life > 1000)
				burnTimer = 1024;
			if (storedEnergy > neighborStoredEnergy)
			{
				int transfer = storedEnergy - neighborStoredEnergy;
				transfer -= transfer / 2;
				neighborStoredEnergy += transfer;
				storedEnergy -= transfer;
				break;
			}
		}
	}

	// Overcharged - begin explosion
	if (burnTimer < 1000 && storedEnergy >= 100)
		burnTimer = 1024;
	if (burnTimer == 1000)
	{
		sim->part_change_type(i, x, y, PT_LAVA);
		if (carbonationFactor < 3)
		{
			self.temp = 500.f + storedEnergy * 10;
			self.ctype = PT_LITH;
		}
		else
		{
			self.temp = 2000.f + storedEnergy * 10;
			self.ctype = PT_GLAS;
		}
	}
	return 0;
}

int LITH_graphics(GRAPHICS_FUNC_ARGS)
{
	// Exploding lith
	if (cpart->life >= 1000)
	{
		int colour = 0xFFA040;
		*colr = PIXR(colour);
		*colg = PIXG(colour);
		*colb = PIXB(colour);
		*pixel_mode |= PMODE_FLARE | PMODE_GLOW;
	}
	// Charged lith
	else if (cpart->ctype > 0)
	{
		int mult = RNG::Ref().between(cpart->ctype / 3, cpart->ctype) / 15;
		mult = std::min(6, mult);
		*colr -= 30 * mult;
		*colb += 20 * mult;
		*pixel_mode |= PMODE_FLARE | PMODE_GLOW;
	}
	return 0;
}

void LITH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LITH";
	elem->Name = "LITH";
	elem->Colour = PIXPACK(0xB6AABF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.2f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.2f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 14;

	elem->Weight = 17;

	elem->HeatConduct = 70;
	elem->Description = "锂。与水接触会爆炸的反应元素。";
	elem->DetailedDescription = "描述：与水接触会发生爆炸。它吸收 CO2，然后可以转化为 GLAS。与水接触时，它会加热自身并将水变成氢气。在 1000K 时它会爆炸。纯净时可用于制造电池。\n元素参数：\n氢化(Tmp)：\n当 LITH 每接触 WATR，SLTW，DSTW，BUBW 或 WTRV 时它的氢化值会增加 1 时。如果锂在 166.85℃以上与它们会产生爆炸。否则它会将它们转化为 HYGN 并释放热量。当锂内含有电时，产生的反应和热量都会更加剧烈。氢化值存储在 Tmp 值中。\n碳酸化(Tmp2)：\n当 LITH 接触到 CO2 时，每吸收一个 CO2 粒子，其碳酸化值增加 1。碳化值存储在 Tmp2 值中锂电池：当锂足够纯(氢化因子+碳酸化因子<5)时，可作为可充电电池使用。锂通过 PSCN 充电,通过 NSCN 放电。电荷值存储在 Ctype 中，并会将该值平均给与周围的 LITH 粒子。\n杂质：LITH 的杂质由氢化和碳酸化值的总和决定。当杂质达到最大值 10 时，它将停止与 CO2 和水反应(除非它已经燃烧)。\n爆炸：在爆炸阶段，LITH 会启动倒数计时器并释放 FIRE 粒子。如果 LITH 在这种状态下与 OXYG 接触，它会将 OXYG 和自身变成 PLSM，并产生一定的压力。当爆炸计时器结束时，它会变成 LAVA。如果碳酸化值大于3，它将变成熔融的 GLAS，而不是熔融的 LITH。如果它与 FIRE 接触同时它的温度大于 166.85℃，氢化值小于 6 时，LITH 就会爆炸。\n反应：ACID 会把 LITH 变成 HYGN，而不是摧毁它。\n初始温度：22.00℃/295.15K\n导热率：70";

	elem->Properties = TYPE_PART | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 453.65f;
	elem->HighTemperatureTransitionElement = PT_LAVA;
	
	elem->Update = &LITH_update;
	elem->Graphics = &LITH_graphics;
	elem->Init = &LITH_init_element;
}
