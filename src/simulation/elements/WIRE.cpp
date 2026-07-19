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
0:  wire
1:  spark head
2:  spark tail

tmp is previous state, ctype is current state
*/
int WIRE_update(UPDATE_FUNC_ARGS)
{
	int count = 0;
	parts[i].tmp = parts[i].ctype;
	parts[i].ctype = 0;
	if (parts[i].tmp == 1)
	{
		parts[i].ctype = 2;
	}
	else if (parts[i].tmp == 2)
	{
		parts[i].ctype = 0;
	}

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3 && parts[ID(r)].ctype == PT_PSCN)
				{
					parts[i].ctype = 1;
					return 0;
				}
				else if (TYP(r) == PT_NSCN && parts[i].tmp == 1)
					sim->spark_conductive_attempt(ID(r), x+rx, y+ry);
				else if (TYP(r) == PT_WIRE && ((ID(r)) > i ? parts[ID(r)].ctype == 1 : parts[ID(r)].tmp == 1) && !parts[i].tmp)
					count++;
			}
		}
	if (count == 1 || count == 2)
		parts[i].ctype = 1;
	return 0;
}

int WIRE_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->ctype == 0)
	{
		*colr = 255;
		*colg = 204;
		*colb = 0;
		return 0;
	}
	else if (cpart->ctype == 1)
	{
		*colr = 50;
		*colg = 100;
		*colb = 255;
		//*pixel_mode |= PMODE_GLOW;
		return 0;
	}
	else if (cpart->ctype == 2)
	{
		*colr = 255;
		*colg = 100;
		*colb = 50;
		//*pixel_mode |= PMODE_GLOW;
		return 0;
	}
	return 0;
}

void WIRE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WIRE";
	elem->Name = "WWLD";
	elem->Colour = COLPACK(0xFFCC00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 250;
	elem->Latent = 0;
	elem->Description = "WireWorld wires, conducts based on a set of GOL-like rules.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WIRE_update;
	elem->Graphics = &WIRE_graphics;
	elem->Init = &WIRE_init_element;
}
