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

int VIRS_update(UPDATE_FUNC_ARGS)
{
	//tmp3 measures how many frames until it is cured (0 if still actively spreading and not being cured)
	//tmp4 measures how many frames until it dies
	int rndstore = RNG::Ref().gen();
	if (parts[i].tmp3)
	{
		parts[i].tmp3 -= (rndstore&0x1) ? 0:1;
		//has been cured, so change back into the original element
		if (parts[i].tmp3 <= 0)
		{
			part_change_type(i,x,y,parts[i].tmp2);
			parts[i].tmp2 = 0;
			parts[i].tmp3 = 0;
			parts[i].tmp4 = 0;
			return 1;
		}

		//cured virus isn't allowed in below code
		return 0;
	}
	//decrease tmp4 so it slowly dies
	if (parts[i].tmp4 > 0)
	{
		if (!(rndstore & 0x7) && --parts[i].tmp4 <= 0)
		{
			sim->part_kill(i);
			return 1;
		}
		rndstore >>= 3;
	}

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				//spread "being cured" state
				if (parts[ID(r)].tmp3 && (TYP(r) == PT_VIRS || TYP(r) == PT_VRSS || TYP(r) == PT_VRSG))
				{
					parts[i].tmp3 = parts[ID(r)].tmp3 + ((rndstore & 0x3) ? 2:1);
					return 0;
				}
				//soap cures virus
				else if (TYP(r) == PT_SOAP)
				{
					parts[i].tmp3 += 10;
					if (!(rndstore & 0x3))
						sim->part_kill(ID(r));
					return 0;
				}
				else if (TYP(r) == PT_PLSM)
				{
					if (surround_space && 10 + RNG::Ref().chance(sim->air->pv[(y+ry)/CELL][(x+rx)/CELL], 100))
					{
						sim->part_create(i, x, y, PT_PLSM);
						return 1;
					}
				}
				else if (TYP(r) != PT_VIRS && TYP(r) != PT_VRSS && TYP(r) != PT_VRSG && TYP(r) != PT_BASE && !(sim->elements[TYP(r)].Properties&PROP_INDESTRUCTIBLE))
				{
					if (!(rndstore & 0x7))
					{
						parts[ID(r)].tmp2 = TYP(r);
						parts[ID(r)].tmp3 = 0;
						if (parts[i].tmp4)
							parts[ID(r)].tmp4 = parts[i].tmp4 + 1;
						else
							parts[ID(r)].tmp4 = 0;
						if (parts[ID(r)].temp < 305.0f)
							sim->part_change_type(ID(r), x + rx, y + ry, PT_VRSS);
						else if (parts[ID(r)].temp > 673.0f)
							sim->part_change_type(ID(r), x + rx, y + ry, PT_VRSG);
						else
							sim->part_change_type(ID(r), x + rx, y + ry, PT_VIRS);
					}
					rndstore >>= 3;
				}
				// Protons make VIRS last forever
				else if (TYP(photons[y+ry][x+rx]) == PT_PROT)
				{
					parts[i].tmp4 = 0;
				}
			}
			// Reset rndstore only once, halfway through
			else if (!rx && !ry)
				rndstore = RNG::Ref().gen();
		}
	return 0;
}

int VIRS_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_BLUR;
	*pixel_mode |= NO_DECO;
	return 1;
} 

void VIRS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VIRS";
	elem->Name = "VIRS";
	elem->Colour = COLPACK(0xFE11F6);
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

	elem->Weight = 31;

	elem->DefaultProperties.temp = 72.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "病毒。将接触到的一切都变成病毒。";
	elem->DetailedDescription = "描述：89.0 版本后加入，会将其碰触到的所有物质变成病毒(VIRS)，一段时间后会自己死亡。肥皂(SOAP)可以治愈病毒(VIRS)并使物质恢复。质子(PROT)可以使病毒(VIRS)不会自动死亡。只能被等离子体(PLSM)点燃。不受 VIRS 影响的元素是能量类型元素引力子(GRVT)、PROT、电子(ELEC)、光子(PHOT)、中子(NEUT)、奇点\n(SING)、反物质(AMTR)和钻石 (DMND)。VIRS 是少数对 LOLZ 和 LOVE 产生有趣效果的元素之一。如果 VIRS 触及其中之一，则 VIRS 将被“克隆”，因为 VIRS 将 LOVE/LOLZ 的一个像素更改为更多 VIRS，但随后LOVE/LOLZ 会重组，将VIRS 推开。最终，VIRS 会同时腐蚀所有的 LOVE/LOLZ，否则 VIRS 将被完全推开并停止被克隆。\n沸点：399.85℃/673K 变成病毒气(VRSG)\n凝固点：31.85℃/305K 变成病毒块(VRSS)\n元素参数：Tmp2=感染物质的 Type 值\n导热率：251/251/251\n初始温度：72.00℃/345.15K";

	elem->Properties = TYPE_LIQUID;
	elem->CarriesTypeIn = 1U << FIELD_TMP2;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 305.0f;
	elem->LowTemperatureTransitionElement = PT_VRSS;
	elem->HighTemperatureTransitionThreshold = 673.0f;
	elem->HighTemperatureTransitionElement = PT_VRSG;

	elem->DefaultProperties.tmp4 = 250;

	elem->Update = &VIRS_update;
	elem->Graphics = &VIRS_graphics;
	elem->Init = &VIRS_init_element;
}
