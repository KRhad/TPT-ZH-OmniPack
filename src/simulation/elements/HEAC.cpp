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

#include <algorithm>
#include <functional>
#include "simulation/ElementsCommon.h"

static const auto isInsulator = [](Simulation* sim, int p) -> bool {
	return p && sim->IsHeatInsulator(sim->parts[ID(p)]);
};

// If this is used elsewhere (GOLD), it should be moved into Simulation.h
template<class BinaryPredicate>
bool CheckLine(Simulation* sim, int x1, int y1, int x2, int y2, BinaryPredicate func)
{
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy;
	float e, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
		{
			if (func(sim, pmap[x][y])) return true;
		}
		else
		{
			if (func(sim, pmap[y][x])) return true;
		}
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if ((y1<y2) ? (y<=y2) : (y>=y2))
			{
				if (reverseXY)
				{
					if (func(sim, pmap[x][y])) return true;
				}
				else
				{
					if (func(sim, pmap[y][x])) return true;
				}
			}
			e -= 1.0f;
		}
	}
	return false;
}

int HEAC_update(UPDATE_FUNC_ARGS)
{
	const int rad = 4;
	int rry, rrx, r;
	float c_heat = 0.0f;
	float hc_total = 0.0f;

	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			rry = ry * rad;
			rrx = rx * rad;
			if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !CheckLine(sim, x, y, x+rrx, y+rry, isInsulator))
			{
				r = pmap[y+rry][x+rrx];
				if (r && !sim->IsHeatInsulator(parts[ID(r)]))
				{
					c_heat += parts[ID(r)].temp;
					hc_total += sim->HeatCapacityOf(parts[ID(r)]);
				}
				r = photons[y+rry][x+rrx];
				if (r && !sim->IsHeatInsulator(parts[ID(r)]))
				{
					c_heat += parts[ID(r)].temp;
					hc_total += sim->HeatCapacityOf(parts[ID(r)]);
				}
			}
		}
	}

	if (hc_total > 0.0f)
	{
		auto pt = restrict_flt(c_heat / hc_total, MIN_TEMP, MAX_TEMP);
		parts[i].temp = pt;

		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				rry = ry * rad;
				rrx = rx * rad;
				if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !CheckLine(sim, x, y, x+rrx, y+rry, isInsulator))
				{
					r = pmap[y+rry][x+rrx];
					if (r && !sim->IsHeatInsulator(parts[ID(r)]))
					{
						parts[ID(r)].temp = pt;
					}
					r = photons[y+rry][x+rrx];
					if (r && !sim->IsHeatInsulator(parts[ID(r)]))
					{
						parts[ID(r)].temp = pt;
					}
				}
			}
		}
	}

	return 0;
}

void HEAC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_HEAC";
	elem->Name = "HEAC";
	elem->Colour = PIXPACK(0xCB6351);
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
	elem->Meltable = 1;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Description = "Rapid heat conductor.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	// can't melt by normal heat conduction, this is used by other elements for special melting behavior
	elem->HighTemperatureTransitionThreshold = 1887.15f;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &HEAC_update;
}
