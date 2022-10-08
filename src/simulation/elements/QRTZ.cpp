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

int QRTZ_update(UPDATE_FUNC_ARGS)
{
	int r, tmp, trade, rx, ry, np, t = parts[i].type;
	if (t == PT_QRTZ)
	{
		int press = int(sim->air->pv[y/CELL][x/CELL] * 64);
		auto diffTolerance = parts[i].temp * 1.0666f;
		if (press - parts[i].tmp3 > diffTolerance || press - parts[i].tmp3 < -diffTolerance)
		{
			part_change_type(i,x,y,PT_PQRT);
			parts[i].life = 5; //timer before it can grow or diffuse again
		}
		parts[i].tmp3 = press;
	}
	if (parts[i].life > 5)
		parts[i].life = 5;
	// absorb SLTW
	if (parts[i].ctype != -1)
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					else if (TYP(r)==PT_SLTW && RNG::Ref().chance(1, 500))
					{
						sim->part_kill(ID(r));
						parts[i].tmp++;
					}
				}
	// grow and diffuse if absorbed SLTW
	if (parts[i].tmp > 0 && (parts[i].vx*parts[i].vx + parts[i].vy*parts[i].vy)<0.2f && parts[i].life<=0)
	{
		bool stopgrow = false;
		int rnd, sry, srx;
		for (trade = 0; trade < 9; trade++)
		{
			rnd = RNG::Ref().between(0, 0x3FF);
			rx = (rnd%5)-2;
			srx = (rnd%3)-1;
			rnd >>= 3;
			ry = (rnd%5)-2;
			sry = (rnd%3)-1;
			if (BOUNDS_CHECK && (rx || ry))
			{
				if (!stopgrow)
				{
					if (!pmap[y+sry][x+srx] && parts[i].tmp != 0)
					{
						np = sim->part_create(-1,x+srx,y+sry,PT_QRTZ);
						if (np > -1)
						{
							parts[np].temp = parts[i].temp;
							parts[np].tmp2 = parts[i].tmp2;
							if (RNG::Ref().chance(1, 2))
							{
								int tmp2 = parts[np].tmp2 + RNG::Ref().between(-1, 1);
								parts[np].tmp2 = tmp2 < 0 ? 0 : tmp2 > 10? 10 : tmp2;
							}
							parts[i].tmp--;
							if (t == PT_PQRT)
							{
								// If PQRT is stationary and has started growing particles of QRTZ, the PQRT is basically part of a new QRTZ crystal. So turn it back into QRTZ so that it behaves more like part of the crystal.
								sim->part_change_type(i,x,y,PT_QRTZ);
							}
							if (RNG::Ref().chance(1, 2))
							{
								parts[np].tmp = -1;//dead qrtz
							}
							else if (!parts[i].tmp && RNG::Ref().chance(1, 15))
							{
								parts[i].tmp=-1;
							}
							stopgrow = true;
						}
					}
				}
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==t && (parts[i].tmp > parts[ID(r)].tmp) && parts[ID(r)].tmp>=0)//diffusion
				{
					tmp = parts[i].tmp - parts[ID(r)].tmp;
					if (tmp == 1)
					{
						parts[ID(r)].tmp ++;
						parts[i].tmp --;
						break;
					}
					if (tmp > 0)
					{
						parts[ID(r)].tmp += tmp/2;
						parts[i].tmp -= tmp/2;
						break;
					}
				}
			}
		}
	}
	return 0;
}

int QRTZ_graphics(GRAPHICS_FUNC_ARGS)
{
	int t = cpart->type, z = cpart->tmp2 - 5; // speckles!
	float transitionTemp = sim->elements[t].HighTemperatureTransitionThreshold;
	// hotglow for quartz
	if (cpart->temp > transitionTemp - 800.0f)
	{
		float frequency = M_PI / (transitionTemp + 800.0f);
		int q = (int)(cpart->temp > transitionTemp ? 800.0f : cpart->temp - (transitionTemp - 800.0f));
		*colr += (int)(sin(frequency*q) * 226 + (z * 16));
		*colg += (int)(sin(frequency*q*4.55 +3.14) * 34 + (z * 16));
		*colb += (int)(sin(frequency*q*2.22 +3.14) * 64 + (z * 16));
	}
	else
	{
		*colr += z * 16;
		*colg += z * 16;
		*colb += z * 16;
	}
	return 0;
}

void QRTZ_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp2 = RNG::Ref().between(0, 10);
	sim->parts[i].tmp3 = int(sim->air->pv[y/CELL][x/CELL] * 64);
}

void QRTZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_QRTZ";
	elem->Name = "QRTZ";
	elem->Colour = COLPACK(0xAADDDD);
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
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 3;
	elem->Latent = 0;
	elem->Description = "Quartz, breakable mineral. Conducts but becomes brittle at lower temperatures.";

	elem->Properties = TYPE_SOLID|PROP_HOT_GLOW|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2573.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &QRTZ_update;
	elem->Graphics = &QRTZ_graphics;
	elem->Func_Create = &QRTZ_create;
	elem->Init = &QRTZ_init_element;
}
