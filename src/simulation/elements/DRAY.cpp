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
#include "simulation/elements/FIGH.h"

int DRAY_update(UPDATE_FUNC_ARGS)
{
	int ctype = TYP(parts[i].ctype), ctypeExtra = ID(parts[i].ctype), copyLength = parts[i].tmp, copySpaces = parts[i].tmp2;
	if (copySpaces < 0)
		copySpaces = parts[i].tmp2  = 0;
	if (copyLength < 0)
		copyLength = parts[i].tmp  = 0;
	else if (copyLength > 0)
		copySpaces++; //strange hack

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				// Spark found, start creating
				if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3)
				{
					bool overwrite = parts[ID(r)].ctype == PT_PSCN;
					// Positions where the line will start being copied at
					int partsRemaining = copyLength, xCopyTo = -1, yCopyTo = -1;
					int localCopyLength = copyLength;

					// INWR doesn't spark from diagonals
					if (parts[ID(r)].ctype == PT_INWR && rx && ry)
						continue;

					// Figure out where the copying will start/end
					bool foundParticle = false;
					bool isEnergy = false;
					for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; ; xCurrent+=xStep, yCurrent+=yStep)
					{
						// Out of bounds, stop looking and don't copy anything
						if (!sim->InBounds(xCurrent, yCurrent))
							break;
						int rr;
						// Haven't found a particle yet, keep looking for one
						// The first particle it sees decides whether it will copy energy particles or not
						if (!foundParticle)
						{
							rr = pmap[yCurrent][xCurrent];
							if (!rr)
							{
								rr = photons[yCurrent][xCurrent];
								if (rr)
									foundParticle = isEnergy = true;
							}
							else
								foundParticle = true;
						}
						// Now that it knows what kind of particle it is copying, do some extra stuff here so we can determine when to stop
						if ((ctype && sim->elements[ctype].Properties&TYPE_ENERGY) || isEnergy)
							rr = photons[yCurrent][xCurrent];
						else
							rr = pmap[yCurrent][xCurrent];

						// Checks for when to stop:
						//  1: If .tmp isn't set, and the element in this spot is the ctype, then stop
						//  2: If .tmp is set, stop when the length limit reaches 0
						if ((!localCopyLength && TYP(rr) == ctype && (ctype != PT_LIFE || parts[ID(rr)].ctype == ctypeExtra))
								|| !--partsRemaining)
						{
							localCopyLength -= partsRemaining;
							xCopyTo = xCurrent + xStep*copySpaces;
							yCopyTo = yCurrent + yStep*copySpaces;
							break;
						}
					}

					// Now, actually copy the particles
					partsRemaining = localCopyLength + 1;
					int type, p;
					for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; sim->InBounds(xCopyTo, yCopyTo) && --partsRemaining; xCurrent+=xStep, yCurrent+=yStep, xCopyTo+=xStep, yCopyTo+=yStep)
					{
						// Get particle to copy
						if (isEnergy)
							type = TYP(photons[yCurrent][xCurrent]);
						else
							type = TYP(pmap[yCurrent][xCurrent]);

						// If sparked by PSCN, overwrite whatever is in the target location, instead of just ignoring it
						if (overwrite)
						{
							if (isEnergy)
							{
								if (photons[yCopyTo][xCopyTo])
									sim->part_kill(ID(photons[yCopyTo][xCopyTo]));
							}
							else
							{
								if (pmap[yCopyTo][xCopyTo])
									sim->part_kill(ID(pmap[yCopyTo][xCopyTo]));
							}
						}
						// Spark hack
						if (type == PT_SPRK)
							p = sim->part_create(-1, xCopyTo, yCopyTo, PT_METL);
						else if (type == PT_STKM || type == PT_STKM2)
							continue; // do not try to copy these non-copyable particles
						else if (type)
							p = sim->part_create(-1, xCopyTo, yCopyTo, type);
						else
							continue;

						// If new particle was created successfully
						if (p >= 0)
						{
							// Spark hack
							if (type == PT_SPRK)
								sim->part_change_type(p, xCopyTo, yCopyTo, PT_SPRK);
							if (isEnergy)
								parts[p] = parts[ID(photons[yCurrent][xCurrent])];
							else if (type != PT_FIGH)
								parts[p] = parts[ID(pmap[yCurrent][xCurrent])];
							else
							{
								// FIGH needs special rules
								const auto& other = parts[ID(pmap[yCurrent][xCurrent])];

								// need to keep .tmp consistent: it points to fighter metadata
								auto old_tmp = parts[p].tmp;
								parts[p] = other;
								parts[p].tmp = old_tmp;

								// Update the new fighter's metadata
								// Do not attempt to copy kinematics of legs - overwritten next frame anyway
								if (other.tmp >= 0 && other.tmp < static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).MaxFighters())
								{
									const Stickman *source_pst = static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Get(other.tmp);
									Stickman *this_pst = static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Get(old_tmp);
									this_pst->rocketBoots = source_pst->rocketBoots;
									this_pst->fan = source_pst->fan;
									this_pst->elem = source_pst->elem;
								}
							}

							parts[p].x = (float)xCopyTo;
							parts[p].y = (float)yCopyTo;
						}
					}
				}
			}
	return 0;
}

void DRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DRAY";
	elem->Name = "DRAY";
	elem->Colour = COLPACK(0xFFAA22);
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

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Duplicator ray. Replicates a line of particles in front of it.";

	elem->Properties = TYPE_SOLID;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DRAY_update;
	elem->Graphics = NULL;
	elem->CtypeDraw = &ctypeDrawVInCtype;
	elem->Init = &DRAY_init_element;
}

