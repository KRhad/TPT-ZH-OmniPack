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

int DSTW_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				switch (TYP(r))
				{
				case PT_SALT:
					if (RNG::Ref().chance(1, 50))
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						// on average, convert 3 DSTW to SLTW before SALT turns into SLTW
						if (RNG::Ref().chance(1, 3))
							sim->part_change_type(ID(r), x+rx, y+ry, PT_SLTW);
					}
					break;
				case PT_SLTW:
					if (RNG::Ref().chance(1, 2000))
					{
						sim->part_change_type(i, x, y, PT_SLTW);
					}
					// no break here intentionally
				case PT_WATR:
					if (RNG::Ref().chance(1, 100))
					{
						sim->part_change_type(i, x, y, PT_WATR);
					}
					break;
				case PT_RBDM:
				case PT_LRBD:
					if ((legacy_enable || parts[i].temp > 12.0f) && RNG::Ref().chance(1, 100))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 4;
					}
					break;
				case PT_FIRE:
					sim->part_kill(ID(r));
					if (RNG::Ref().chance(1, 30))
					{
						sim->part_kill(i);
						return 1;
					}
					break;
				case PT_SMKE: //DSTW + SMKE = BASE
					if (parts[ID(r)].temp > (40 + 273.15f) && parts[ID(r)].temp < (60 + 273.15f) &&
						parts[i].temp > (40 + 273.15f) && parts[i].temp < (60 + 273.15f))
					{
						if (RNG::Ref().chance(1, 100))
						{
							sim->part_change_type(i,x,y,PT_BASE);
							parts[i].life = 1;
							sim->part_kill(ID(r));
						}
					}
					break;
				default:
					continue;
				}
			}
	return 0;
}

void DSTW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DSTW";
	elem->Name = "DSTW";
	elem->Colour = COLPACK(0x1020C0);
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
	elem->HeatConduct = 23;
	elem->Latent = 7500;
	elem->Description = "蒸馏水，不导电。";
	elem->DetailedDescription = "描述：不导电的理论纯水，由水蒸气冷凝或中子轰击水(WATR)获得，DSTW 具有与 WATR 相同的沸点和冰点。植物(PLNT)不能吸收这种水且不会导致铁生锈。DSTW 可以被 BUBW 和 WATR 污染成 WATR。当通电时会发生电解(分离成 HHO)。\n沸点：99.85℃/373.0K\n凝固点：0℃/273.15K\n导热率：23\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_LIQUID | PROP_NEUTPASS | PROP_PHOTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 273.15f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &DSTW_update;
	elem->Graphics = NULL;
	elem->Init = &DSTW_init_element;
}
