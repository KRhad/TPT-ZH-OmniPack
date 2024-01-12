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

static void wtrv_reactions(int wtrv1_id, UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y + ry][x + rx];
				if (!r || ID(r) == wtrv1_id)
					continue;
				int rt = TYP(r);

				// WTRV + BCOL -> OIL
				if (rt == PT_BCOL && parts[ID(r)].temp > 200.0f + 273.15f && parts[wtrv1_id].temp > 200.0f + 273.15f && sim->air->pv[(y + ry) / CELL][(x + rx) / CELL] > 7.f)
				{
					sim->part_change_type(ID(r), x + rx, y + ry, PT_OIL);
					sim->part_kill(wtrv1_id);
					return;
				}
			}
		}
	}
}

static void hygn_reactions(int hygn1_id, UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y + ry][x + rx];
				if (!r || ID(r) == hygn1_id)
					continue;
				int rt = TYP(r);

				// HYGN + DESL -> OIL + WATR
				if (rt == PT_DESL)
				{
					sim->part_change_type(ID(r), x + rx, y + ry, PT_WATR);
					sim->part_change_type(hygn1_id, (int)(parts[hygn1_id].x + 0.5f), (int)(parts[hygn1_id].y + 0.5f), PT_OIL);
					return;
				}

				// HYGN + OXYG -> DSTW + SPRK + Heat
				if (rt == PT_O2 && !parts[i].life)
				{
					sim->part_change_type(ID(r), x + rx, y + ry, PT_DSTW);
					sim->part_change_type(hygn1_id, (int)(parts[hygn1_id].x + 0.5f), (int)(parts[hygn1_id].y + 0.5f), PT_DSTW);
					parts[ID(r)].temp += 5.0f;
					parts[hygn1_id].temp += 5.0f;

					parts[i].ctype = PT_PTNM;
					parts[i].life = 4;
					sim->part_change_type(i, x, y, PT_SPRK);
					return;
				}

				// Cold fusion: 2 hydrogen > 500 C has a chance to fuse
				if (rt == PT_H2 && RNG::Ref().chance(1, 1000) && parts[ID(r)].temp > 500.0f + 273.15f && parts[hygn1_id].temp > 500.0f + 273.15f)
				{
					sim->part_change_type(ID(r), x + rx, y + ry, PT_NBLE);
					sim->part_change_type(hygn1_id, (int)(parts[hygn1_id].x + 0.5f), (int)(parts[hygn1_id].y + 0.5f), PT_NEUT);

					parts[ID(r)].temp += 1000.0f;
					parts[hygn1_id].temp += 1000.0f;
					sim->air->pv[(y + ry) / CELL][(x + rx) / CELL] += 10.0f;

					int j = sim->part_create(-3, x + rx, y + ry, PT_PHOT);
					if (j > -1)
					{
						parts[j].ctype = 0x7C0000;
						parts[j].temp = parts[ID(r)].temp;
						parts[j].tmp = 0x1;
					}
					if (RNG::Ref().chance(1, 10))
					{
						int j = sim->part_create(-3, x + rx, y + ry, PT_ELEC);
						if (j > -1)
							parts[j].temp = parts[ID(r)].temp;
					}
					return;
				}
			}
		}
	}
}

int PTNM_update(UPDATE_FUNC_ARGS)
{
	int hygn1_id = -1; // Id of a hydrogen particle for hydrogen multi-particle reactions
	int wtrv1_id = -1; // same but wtrv

	// Fast conduction (like GOLD)
	if (!parts[i].life)
	{
		for (int j = 0; j < 4; j++)
		{
			static const int checkCoordsX[] = { -4, 4, 0, 0 };
			static const int checkCoordsY[] = { 0, 0, -4, 4 };
			int rx = checkCoordsX[j];
			int ry = checkCoordsY[j];
			int r = pmap[y + ry][x + rx];
			if (r && TYP(r) == PT_SPRK && parts[ID(r)].life && parts[ID(r)].life < 4)
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].life = 4;
				parts[i].ctype = PT_PTNM;
			}
		}
	}

	// Single element reactions
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				int r = pmap[y + ry][x + rx];
				if (!r)
					continue;
				int rt = TYP(r);

				if (rt == PT_H2 && hygn1_id < 0)
					hygn1_id = ID(r);

				if (rt == PT_WTRV && wtrv1_id < 0)
					wtrv1_id = ID(r);

				// These reactions will occur instantly in contact with PTNM
				// --------------------------------------------------------

				// Shield instantly grows (even without SPRK)
				if (!parts[ID(r)].life && (rt == PT_SHLD1 || rt == PT_SHLD2 || rt == PT_SHLD3))
				{
					int next = PT_SHLD1;
					switch (rt)
					{
					case PT_SHLD1: next = PT_SHLD2; break;
					case PT_SHLD2: next = PT_SHLD3; break;
					case PT_SHLD3: next = PT_SHLD4; break;
					}
					sim->part_change_type(ID(r), x + rx, y + ry, next);
					parts[ID(r)].life = 7;
					continue;
				}

				// ISZS / ISOZ -> PHOT + PLUT
				if (rt == PT_ISZS || rt == PT_ISOZ)
				{
					sim->part_change_type(ID(r), x + rx, y + ry, PT_PLUT);
					sim->part_create(-3, x + rx, y + ry, PT_PHOT);
					continue;
				}

				// These reactions are dependent on temperature
				// Probability goes quadratically from 0% / frame to 100% / frame from 0 C to 1500 C
				// --------------------------------------------------------
				float prob = std::min(1.0f, parts[i].temp / (273.15f + 1500.0f));
				prob *= prob;

				if (RNG::Ref().uniform01() <= prob)
				{
					switch (rt)
					{
					case PT_GAS: // GAS + > 2 pressure + >= 200 C -> INSL
						if (parts[ID(r)].temp >= 200.0f + 273.15f && sim->air->pv[(y + ry) / CELL][(x + rx) / CELL] > 2.0f)
						{
							sim->part_change_type(ID(r), x + rx, y + ry, PT_INSL);
							parts[i].temp += 60.0f; // Other part is INSL, adding temp is useless
						}
						break;

					case PT_BREL: // BREL + > 1000 C + > 50 pressure -> EXOT
						if (parts[ID(r)].temp > 1000.0f + 273.15f && sim->air->pv[(y + ry) / CELL][(x + rx) / CELL] > 50.0f)
						{
							sim->part_change_type(ID(r), x + rx, y + ry, PT_EXOT);
							parts[ID(r)].temp -= 30.0f;
							parts[i].temp -= 30.0f;
						}
						break;

					case PT_SMKE: // SMKE -> CO2
						sim->part_change_type(ID(r), x + rx, y + ry, PT_CO2);
						break;
					}
				}
			}
		}
	}

	// Hydrogen reactions
	if (hygn1_id >= 0)
	{
		hygn_reactions(hygn1_id, UPDATE_FUNC_SUBCALL_ARGS);
	}

	// WTRV reactions
	if (wtrv1_id >= 0)
	{
		wtrv_reactions(wtrv1_id, UPDATE_FUNC_SUBCALL_ARGS);
	}

	return 0;
}

int PTNM_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->tmp)
		*pixel_mode |= PMODE_FLARE;
	return 0;
}

void PTNM_create(ELEMENT_CREATE_FUNC_ARGS)
{
	if (RNG::Ref().chance(1, 15))
		sim->parts[i].tmp = 1;
}

void PTNM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PTNM";
	elem->Name = "PTNM";
	elem->Colour = PIXPACK(0xD5E0EB);
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
	elem->Description = "Platinum. Catalyzes certain reactions.";

	elem->Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_HOT_GLOW | PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1768.0f + 273.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &PTNM_update;
	elem->Graphics = &PTNM_graphics;
	elem->Func_Create = &PTNM_create;
	elem->Init = &PTNM_init_element;
}
