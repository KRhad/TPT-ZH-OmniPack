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

int PUMP_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life == 10)
	{
		if (parts[i].temp >= 256.0f + 273.15f)
			parts[i].temp = 256.0f + 273.15f;
		else if (parts[i].temp <= -256.0f + 273.15f)
			parts[i].temp = -256.0f + 273.15f;

		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
			{
				if (parts[i].tmp != 1)
				{
					if (!(rx && ry))
						sim->air->pv[(y/CELL)+ry][(x/CELL)+rx] += 0.1f*((parts[i].temp-273.15f)-sim->air->pv[(y/CELL)+ry][(x/CELL)+rx]);
				}
				else
				{
					int r = pmap[y+ry][x+rx];
					if (TYP(r) == PT_FILT)
					{
						int newPressure = parts[ID(r)].ctype - 0x10000000;
						if (newPressure >= 0 && newPressure <= 512)
						{
							sim->air->pv[(y + ry) / CELL][(x + rx) / CELL] = newPressure - 256;
						}
					}
				}
			}
	}
	return 0;
}

int PUMP_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod = ((cpart->life>10?10:cpart->life)*19);
	*colb += lifemod;
	return 0;
}

void PUMP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PUMP";
	elem->Name = "PUMP";
	elem->Colour = COLPACK(0x0A0A3B);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 9;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Pressure pump. Changes pressure to its temp when activated. (use HEAT/COOL).";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 10;

	elem->Update = &PUMP_update;
	elem->Graphics = &PUMP_graphics;
	elem->Init = &PUMP_init_element;
}
