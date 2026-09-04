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

int ACEL_update(UPDATE_FUNC_ARGS)
{
	float multiplier;
	if (parts[i].life != 0)
	{
		float change = (float)(parts[i].life > 1000 ? 1000 : (parts[i].life < 0 ? 0 : parts[i].life));
		multiplier = 1.0f+(change/100.0f);
	}
	else
	{
		multiplier = 1.1f;
	}
	parts[i].tmp = 0;
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (!rx != !ry)
			{
				int r = pmap[y+ry][x+rx];
				if(!r)
					r = photons[y+ry][x+rx];
				if (!r)
					continue;
				if (sim->elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY))
				{
					auto vx = parts[ID(r)].vx;
					auto vy = parts[ID(r)].vy;

					vx *= multiplier;
					vy *= multiplier;

					// Restrict velocity and try to preserve direction
					auto mv = fmaxf(fabsf(vx), fabsf(vy));
					if (mv > MAX_VELOCITY)
					{
						vx *= MAX_VELOCITY/mv;
						vy *= MAX_VELOCITY/mv;
					}

					parts[ID(r)].vx = vx;
					parts[ID(r)].vy = vy;

					parts[i].tmp = 1;
				}
			}
	return 0;
}

int ACEL_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp)
		*pixel_mode |= PMODE_GLOW;
	return 0;
}

void ACEL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ACEL";
	elem->Name = "ACEL";
	elem->Colour = COLPACK(0x0099CC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
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
	elem->Description = "加速器，加速附近的元素。";
	elem->DetailedDescription = "描述：可以加速物质(除了固体)，有效范围 1 个像素，默认状态下能加速粒子 10%的速度，通过修改 Life 值可以改变加速程度。将直行或列靠在一起(1 像素间隙)并在它们之间放置一个粒子。粒子将在它移动的方向上加速。此外，加速器不会克服重力。\n元素参数：Life 值(0-1000)非零时，加速程度从 0.01%-10%范围内改变，负值时为减速，Life=0 时默认加速 10%\n导热率：251\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &ACEL_update;
	elem->Graphics = &ACEL_graphics;
	elem->Init = &ACEL_init_element;
}
