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

#define COOLDOWN 15
#define LIMIT 5

int POLO_update(UPDATE_FUNC_ARGS)
{
	int r = photons[y][x];
	if (parts[i].tmp < LIMIT && !parts[i].life)
	{
		if (RNG::Ref().chance(1, 10000) && !parts[i].tmp)
		{
			int s = sim->part_create(-3, x, y, PT_NEUT);
			if (s >= 0)
			{
				parts[i].life = COOLDOWN;
				parts[i].tmp++;

				parts[i].temp = ((parts[i].temp + parts[s].temp) + 600.0f) / 2.0f;
				parts[s].temp = parts[i].temp;
			}
		}

		if (r && RNG::Ref().chance(1, 100))
		{
			int s = sim->part_create(-3, x, y, PT_NEUT);
			if (s >= 0)
			{
				parts[i].temp = ((parts[i].temp + parts[ID(r)].temp + parts[ID(r)].temp) + 600.0f) / 3.0f;
				parts[i].life = COOLDOWN;
				parts[i].tmp++;

				parts[ID(r)].temp = parts[i].temp;

				parts[s].temp = parts[i].temp;
				parts[s].vx = parts[ID(r)].vx;
				parts[s].vy = parts[ID(r)].vy;
			}
		}
	}
	if (parts[i].tmp2 >= 10)
	{
		sim->part_change_type(i,x,y,PT_PLUT);
		parts[i].temp = (parts[i].temp+600.0f)/2.0f;
		return 1;
	}
	if (TYP(r) == PT_PROT)
	{
		parts[i].tmp2++;
		sim->part_kill(ID(r));
	}
	if (parts[i].temp < 388.15f)
	{
		parts[i].temp += 0.2f;
	}
	return 0;
}

int POLO_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp >= LIMIT)
	{
		*colr = 0x70;
		*colg = 0x70;
		*colb = 0x70;
	}
	else
		*pixel_mode |= PMODE_GLOW;

	return 0;
}

void POLO_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_POLO";
	elem->Name = "POLO";
	elem->Colour = PIXPACK(0x506030);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.4f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 0;
	elem->PhotonReflectWavelengths = 0x000FF200;

	elem->Weight = 90;

	elem->DefaultProperties.temp = 388.15f; 
	elem->HeatConduct = 251;
	elem->Description = "钋，高放射性。衰变成 NEUT 并升温。";
	elem->DetailedDescription = "描述：钋，高放射性。衰变成 NEUT 并升温。钋除了随着时间的推移固有的热量增加外，还会以恒定的速率产生高温中子。暴露于中子会增加其 Tmp 值，在 5 时变为贫化钋。贫化钋呈灰色。耗尽的钋不会释放中子，但会继续释放热量，最高温度为 115.01℃。如果钋的 Tmp 低于默认值，它会产生更长的中子。将 Tmp 设置为-50将使钋产生很长时间的 NEUT，因为它会在耗尽之前释放 55 个中子。鉴于钋对质子不透明，钋在暴露于质子时会变成钚，但这可能比预期花费的时间更长。\n导热率：251\n初始温度：115℃/388.15K";

	elem->Properties = TYPE_PART|PROP_NEUTPASS|PROP_RADIOACTIVE|PROP_LIFE_DEC|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 526.95f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &POLO_update;
	elem->Graphics = &POLO_graphics;
}
