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
#include "simulation/CoordStack.h"

// INST that can be sparked
bool contains_sparkable_INST(Simulation *sim, int x, int y)
{
	return TYP(pmap[y][x]) == PT_INST && parts[ID(pmap[y][x])].life <= 0;
}

// Any INST or SPRK(INST) regardless of life
bool part_cmp_conductive(unsigned int p, int t)
{
	return (TYP(p) == (unsigned int)t || (TYP(p)==PT_SPRK && parts[ID(p)].ctype == t));
}

int INST_flood_spark(Simulation *sim, int x, int y)
{
	int x1, x2;
	const int cm = PT_INST;
	int created_something = 0;

	if (!contains_sparkable_INST(sim, x, y))
		return 0;

	try
	{
		CoordStack cs;
		cs.push(x, y);

		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			// go left as far as possible
			while (x1>=CELL)
			{
				if (!contains_sparkable_INST(sim, x1-1, y)) break;
				x1--;
			}
			// go right as far as possible
			while (x2<XRES-CELL)
			{
				if (!contains_sparkable_INST(sim, x2+1, y)) break;
				x2++;
			}
			// fill span
			for (x=x1; x<=x2; x++)
			{
				if (contains_sparkable_INST(sim, x, y))
				{
					sim->spark_conductive(ID(pmap[y][x]), x, y);
					created_something = 1;
				}
			}

			// add vertically adjacent pixels to stack
			// (wire crossing for INST)
			if (y>=CELL+1 && x1==x2 &&
					part_cmp_conductive(pmap[y-1][x1-1], cm) &&
					part_cmp_conductive(pmap[y-1][x1], cm) &&
					part_cmp_conductive(pmap[y-1][x1+1], cm) &&
					!part_cmp_conductive(pmap[y-2][x1-1], cm) &&
					part_cmp_conductive(pmap[y-2][x1], cm) &&
					!part_cmp_conductive(pmap[y-2][x1+1], cm))
			{
				// travelling vertically up, skipping a horizontal line
				if (contains_sparkable_INST(sim, x1, y-2))
					cs.push(x1, y-2);
			}
			else if (y>=CELL+1)
			{
				for (x=x1; x<=x2; x++)
				{
					// if at the end of a horizontal section, or if it's a T junction
					if (x==x1 || x==x2 || y>=YRES-CELL-1 || !part_cmp_conductive(pmap[y+1][x],cm) || part_cmp_conductive(pmap[y+1][x-1],cm) || part_cmp_conductive(pmap[y+1][x+1],cm))
					{
						if (contains_sparkable_INST(sim, x, y-1))
							cs.push(x, y-1);
					}
				}
			}

			if (y<YRES-CELL-1 && x1==x2 &&
					part_cmp_conductive(pmap[y+1][x1-1], cm) &&
					part_cmp_conductive(pmap[y+1][x1], cm) &&
					part_cmp_conductive(pmap[y+1][x1+1], cm) &&
					!part_cmp_conductive(pmap[y+2][x1-1], cm) &&
					part_cmp_conductive(pmap[y+2][x1], cm) &&
					!part_cmp_conductive(pmap[y+2][x1+1], cm))
			{
				// travelling vertically down, skipping a horizontal line
				if (contains_sparkable_INST(sim, x1, y+2))
					cs.push(x1, y+2);
			}
			else if (y<YRES-CELL-1)
			{
				for (x=x1; x<=x2; x++)
				{
					if (x==x1 || x==x2 || y<0 || !part_cmp_conductive(pmap[y-1][x],cm) || part_cmp_conductive(pmap[y-1][x-1],cm) || part_cmp_conductive(pmap[y-1][x+1],cm))
					{
						if (contains_sparkable_INST(sim, x, y+1))
							cs.push(x, y+1);
					}
				}
			}
		} while (cs.getSize()>0);
	}
	catch (const CoordStackOverflowException& e)
	{
		(void)e; //ignore compiler warning
		return -1;
	}

	return created_something;
}

void INST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_INST";
	elem->Name = "INST";
	elem->Colour = COLPACK(0x404039);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
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
	elem->Description = "Instantly conducts, PSCN to charge, NSCN to take.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &INST_init_element;
}
