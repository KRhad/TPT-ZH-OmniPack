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

int HSWC_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life == 10 && parts[i].tmp == 1)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						r = photons[y+ry][x+rx];
					if (TYP(r) == PT_FILT)
					{
						int newTemp = parts[ID(r)].ctype - 0x10000000;
						if (newTemp >= MIN_TEMP && newTemp <= MAX_TEMP)
							parts[i].temp = parts[ID(r)].ctype - 0x10000000;
					}
				}
	}
	return 0;
}

int HSWC_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod = ((cpart->life>10?10:cpart->life)*19);
	*colr += lifemod;
	return 0;
}

void HSWC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_HSWC";
	elem->Name = "HSWC";
	elem->Colour = COLPACK(0x3B0A0A);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
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
	elem->Description = "热开关。仅激活时导热。";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &HSWC_update;
	elem->Graphics = &HSWC_graphics;
	elem->Init = &HSWC_init_element;
}
