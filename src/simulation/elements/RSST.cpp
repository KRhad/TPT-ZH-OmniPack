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

int RSST_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx < 2; rx++)
	{
		for (int ry = -1; ry < 2; ry++)
		{
			int r = pmap[y+ry][x+rx];

			if (!r)
				continue;

			// RSST + GUNP = FIRW
			if(TYP(r) == PT_GUNP)
			{
				sim->part_create(i, x, y, PT_FIRW);
				sim->part_kill(ID(r));
				return 1;
			}

			// RSST + BCOL = FSEP
			if(TYP(r) == PT_BCOL)
			{
				sim->part_create(i, x, y, PT_FSEP);
				parts[i].life = 50;
				sim->part_kill(ID(r));
				return 1;
			}

			// Set RSST ctype from nearby clone
			if((TYP(r) == PT_CLNE) || (TYP(r) == PT_PCLN))
			{
				if(parts[ID(r)].ctype != PT_RSST)
					parts[i].ctype = parts[ID(r)].ctype;
			}

			// Set RSST tmp from nearby breakable clone
			if((TYP(r) == PT_BCLN) || (TYP(r) == PT_PBCN))
			{
				if(parts[ID(r)].ctype != PT_RSST)
					parts[i].tmp = parts[ID(r)].ctype;
			}
		}
	}

	return 0;
}

void RSST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RSST";
	elem->Name = "RSST";
	elem->Colour = COLPACK(0xF95B49);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.3f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.15f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 50;

	elem->Weight = 33;

	elem->DefaultProperties.temp = R_TEMP + 20.0f + 273.15f;
	elem->HeatConduct = 55;
	elem->Latent = 0;
	elem->Description = "阻光材料。遇光子固化，电子与火花可破坏。";

	elem->Properties = TYPE_LIQUID | PROP_PHOTPASS | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_NEUTPASS;
	elem->CarriesTypeIn = (1U << FIELD_CTYPE) | (1U << FIELD_TMP);

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &RSST_update;
	elem->Graphics = NULL;
	elem->Init = &RSST_init_element;
}
