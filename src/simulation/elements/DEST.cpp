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

int DEST_update(UPDATE_FUNC_ARGS)
{
	int rx = RNG::Ref().between(-2, 2);
	int ry = RNG::Ref().between(-2, 2);

	int r = pmap[y+ry][x+rx];
	if (!r || TYP(r)==PT_DEST || (sim->elements[TYP(r)].Properties&PROP_INDESTRUCTIBLE) || (sim->elements[TYP(r)].Properties&PROP_CLONE) || (sim->elements[TYP(r)].Properties&PROP_BREAKABLECLONE))
		return 0;

	if (parts[i].life<=0 || parts[i].life>37)
	{
		parts[i].life = RNG::Ref().between(30, 49);
		sim->air->pv[y/CELL][x/CELL]+=60.0f;
	}
	if (TYP(r)==PT_PLUT || TYP(r)==PT_DEUT)
	{
		sim->air->pv[y/CELL][x/CELL]+=20.0f;
		if (RNG::Ref().chance(1, 2))
		{
			sim->part_create(ID(r), x+rx, y+ry, PT_NEUT);
			parts[ID(r)].temp = MAX_TEMP;
			sim->air->pv[y/CELL][x/CELL] += 10.0f;
			parts[i].life-=4;
		}
	}
	else if (TYP(r)==PT_INSL)
	{
		sim->part_create(ID(r), x+rx, y+ry, PT_PLSM);
	}
	else if (RNG::Ref().chance(1, 3))
	{
		sim->part_kill(ID(r));
		parts[i].life -= 4*((sim->elements[TYP(r)].Properties&TYPE_SOLID)?3:1);
		if (parts[i].life<=0)
			parts[i].life=1;
	}
	else if (!sim->IsHeatInsulator(parts[ID(r)]))
		parts[ID(r)].temp = MAX_TEMP;
	parts[i].temp = MAX_TEMP;
	sim->air->pv[y/CELL][x/CELL] = restrict_flt(sim->air->pv[y/CELL][x/CELL] + 80.0f, MIN_PRESSURE, MAX_PRESSURE);
	return 0;
}

int DEST_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life)
	{
		*pixel_mode |= PMODE_LFLARE;
	}
	else
	{
		*pixel_mode |= PMODE_SPARK;
	}
	return 0;
}

void DEST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DEST";
	elem->Name = "DEST";
	elem->Colour = COLPACK(0xFF3311);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = -0.05f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.4f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 101;

	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "可穿透几乎所有物质的更强炸弹。";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DEST_update;
	elem->Graphics = &DEST_graphics;
	elem->Init = &DEST_init_element;
}
