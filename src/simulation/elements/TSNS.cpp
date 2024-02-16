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

int TSNS_update(UPDATE_FUNC_ARGS)
{
	int rd = parts[i].tmp2;
	if (rd > 25)
		parts[i].tmp2 = rd = 25;
	if (parts[i].life)
	{
		parts[i].life = 0;
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						r = photons[y+ry][x+rx];
					if (!r)
						continue;
					int rt = TYP(r);
					int pavg = parts_avg(i, ID(r), PT_INSL);
					if (pavg != PT_INSL && pavg != PT_RSSS)
					{
						if ((sim->elements[rt].Properties&PROP_CONDUCTS) && !(rt == PT_WATR || rt == PT_SLTW || rt == PT_NTCT || rt == PT_PTCT || rt == PT_INWR) && parts[ID(r)].life == 0)
						{
							parts[ID(r)].life = 4;
							parts[ID(r)].ctype = rt;
							sim->part_change_type(ID(r), x+rx, y+ry, PT_SPRK);
						}
					}
				}
	}
	bool setFilt = false;
	int photonWl = 0;
	for (int rx = -rd; rx <= rd; rx++)
		for (int ry = -rd; ry <= rd; ry++)
			if (x + rx >= 0 && y + ry >= 0 && x + rx < XRES && y + ry < YRES && (rx || ry))
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					r = photons[y+ry][x+rx];
				if (!r)
					continue;

				switch (parts[i].tmp)
				{
				// Temperature serialization into FILT
				case 1:
					if (TYP(r) != PT_TSNS && TYP(r) != PT_FILT)
					{
						setFilt = true;
						photonWl = parts[ID(r)].temp;
					}
					break;
				// Invert mode
				case 2:
					if (TYP(r) != PT_TSNS && TYP(r) != PT_METL && parts[ID(r)].temp < parts[i].temp)
						parts[i].life = 1;
					break;
				// Default mode
				case 0:
				default:
					if (TYP(r) != PT_TSNS && TYP(r) != PT_METL && parts[ID(r)].temp > parts[i].temp)
						parts[i].life = 1;
				}
			}
	if (setFilt)
	{
		int nx, ny;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					nx = x + rx;
					ny = y + ry;
					while (TYP(r) == PT_FILT)
					{
						parts[ID(r)].ctype = 0x10000000 + photonWl;
						nx += rx;
						ny += ry;
						if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
							break;
						r = pmap[ny][nx];
					}
				}
	}
	return 0;
}

void TSNS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_TSNS";
	elem->Name = "TSNS";
	elem->Colour = COLPACK(0xFD00D5);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SENSOR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.96f;
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

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Temperature sensor, creates a spark when there's a nearby particle with a greater temperature.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp2 = 2;

	elem->Update = &TSNS_update;
	elem->Graphics = NULL;
	elem->Init = &TSNS_init_element;
}
