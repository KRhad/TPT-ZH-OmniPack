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

int DCEL_update(UPDATE_FUNC_ARGS)
{
	float multiplier = 1.0f/1.1f;
	if (parts[i].life)
		multiplier = 1.0f - ((parts[i].life > 100 ? 100 : (parts[i].life < 0 ? 0 : parts[i].life)) / 100.0f);

	parts[i].tmp = 0;
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if ((rx || ry) && !(rx && ry))
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					r = photons[y+ry][x+rx];
				if ((ID(r)) >= NPART || !r)
					continue;
				if (sim->elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY))
				{
					parts[ID(r)].vx *= multiplier;
					parts[ID(r)].vy *= multiplier;
					parts[i].tmp = 1;
				}
			}
	return 0;
}

int DCEL_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp)
		*pixel_mode |= PMODE_GLOW;
	return 0;
}

void DCEL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DCEL";
	elem->Name = "DCEL";
	elem->Colour = COLPACK(0x99CC00);
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
	elem->Description = "减速器，减慢附近元素的速度。";
	elem->DetailedDescription = "描述：和加速器(ACEL)相反，但是只能减速能量粒子(光子 PHOT、中子 NEUT、电子 ELEC 等)通过修改 Life 值可以改变减速程度。最大值为 100。将 Life 设置为 100 时会使所有粒子立即停止。默认情况下，它将使粒子减速91%。\n导热率：251\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DCEL_update;
	elem->Graphics = &DCEL_graphics;
	elem->Init = &DCEL_init_element;
}
