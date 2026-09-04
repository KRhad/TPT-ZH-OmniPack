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
#include "ETRD.h"

void ETRD_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	// NB: for ETRD countLife0 tracking to work, life value must be set to the new value before calling part_change_type with new type==ETRD

	if (static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).isValid)
	{
		if (from == PT_ETRD && sim->parts[i].life == 0)
			static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).countLife0--;
		if (to == PT_ETRD && sim->parts[i].life == 0)
		{
			static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).countLife0++;
		}
	}
}

void ETRD_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ETRD";
	elem->Name = "ETRD";
	elem->Colour = COLPACK(0x404040);
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
	elem->Description = "电极。产生等离子弧。";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

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
	elem->Func_ChangeType = &ETRD_ChangeType;
	elem->Init = &ETRD_init_element;

	sim->elementData[t].reset(new ETRD_ElementDataContainer);
}

int nearestSparkablePart(Simulation *sim, int targetId)
{
	if (!sim->elementCount[PT_ETRD])
		return -1;
	if (static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).isValid && static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).countLife0 <= 0)
		return -1;

	particle *parts = sim->parts;
	if (parts[targetId].tmp2 && parts[targetId].tmp > parts[targetId].tmp2) // Invalid range if max is set
		return -1;

	const int maxDistance = int(std::hypot(XRES, YRES));
	int foundDistance = parts[targetId].tmp2 ? std::min(parts[targetId].tmp2, maxDistance) : maxDistance; // tmp2 sets max distance
	int foundI = -1;
	Point targetPos = Point((int)parts[targetId].x, (int)parts[targetId].y);

	if (static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).isValid)
	{
		// countLife0 doesn't need recalculating, so just focus on finding the nearest particle

		// If the simulation contains lots of particles, check near the target position first since going through all particles will be slow.
		// Threshold = number of positions checked, *2 because it's likely to access memory all over the place (less cache friendly) and there's extra logic needed
		// TODO: probably not optimal if excessive stacking is used
		std::vector<ETRD_deltaWithLength> deltaPos = static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).deltaPos;
		if (sim->parts_lastActiveIndex > (int)deltaPos.size()*2)
		{
			for (std::vector<ETRD_deltaWithLength>::iterator iter = deltaPos.begin(), end = deltaPos.end(); iter != end; ++iter)
			{
				ETRD_deltaWithLength delta = (*iter);
				Point checkPos = targetPos + delta.d;
				int checkDistance = delta.length;
				if (parts[targetId].tmp >= checkDistance) // tmp sets min distance
				{
					continue;
				}
				if (foundDistance < checkDistance)
				{
					// deltaPos is sorted in order of ascending length, so foundDistance < checkDistance means all later items are further away.
					break;
				}
				if (sim->InBounds(checkPos.X, checkPos.Y) && checkDistance <= foundDistance)
				{
					int r = pmap[checkPos.Y][checkPos.X];
					if (r && TYP(r) == PT_ETRD && !parts[ID(r)].life && ID(r) != targetId && checkDistance < foundDistance)
					{
						foundDistance = checkDistance;
						foundI = ID(r);
					}
				}
			}
		}
		// If neighbor search didn't find a suitable particle, search all particles
		if (foundI < 0)
		{
			for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
			{
				if (parts[i].type == PT_ETRD && !parts[i].life)
				{
					Point checkPos = Point((int)parts[i].x-targetPos.X, (int)parts[i].y-targetPos.Y);
					int checkDistance = int(std::hypot(checkPos.X, checkPos.Y));
					if (checkDistance < foundDistance && checkDistance > parts[targetId].tmp && i != targetId) // tmp sets min distance
					{
						foundDistance = checkDistance;
						foundI = i;
					}
				}
			}
		}
	}
	else
	{
		// Recalculate countLife0, and search for the closest suitable particle
		int countLife0 = 0;
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
		{
			if (parts[i].type == PT_ETRD && !parts[i].life)
			{
				countLife0++;
				Point checkPos = Point((int)parts[i].x-targetPos.X, (int)parts[i].y-targetPos.Y);
				int checkDistance = int(std::hypot(checkPos.X, checkPos.Y));
				if (checkDistance < foundDistance && checkDistance > parts[targetId].tmp && i != targetId) // tmp sets min distance
				{
					foundDistance = checkDistance;
					foundI = i;
				}
			}
		}
		static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).countLife0 = countLife0;
		static_cast<ETRD_ElementDataContainer&>(*sim->elementData[PT_ETRD]).isValid = true;
	}
	return foundI;
}
