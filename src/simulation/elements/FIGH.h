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

#ifndef SIMULATION_ELEMENTS_FIGH_H
#define SIMULATION_ELEMENTS_FIGH_H 

#include <cstring>
#include "powder.h"
#include "simulation/ElementDataContainer.h"
#include "STKM.h"

class FIGH_ElementDataContainer : public ElementDataContainer
{
private:
	static const int maxFighters = 100;
	Stickman fighters[maxFighters];
	int usedCount;
public:	
	FIGH_ElementDataContainer() :
		usedCount(0)
	{
		memset(fighters, 0, sizeof(fighters));
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<FIGH_ElementDataContainer>(*this); }

	Stickman * Get(unsigned char i)
	{
		return fighters+i;
	}

	int Alloc()
	{
		if (usedCount>=maxFighters) return -1;
		int i = 0;
		while (i<maxFighters && fighters[i].spwn==1) i++;
		if (i<maxFighters)
		{
			fighters[i].spwn = 1;
			fighters[i].elem = PT_DUST;
			usedCount++;
			return i;
		}
		else return -1;
	}

	// Mark a specific fighter as allocated - used for portals
	void AllocSpecific(unsigned char i)
	{
		if (!fighters[i].spwn)
		{
			fighters[i].spwn = 1;
			usedCount++;
		}
	}

	void Free(unsigned char i)
	{
		if (fighters[i].spwn)
		{
			fighters[i].spwn = 0;
			usedCount--;
		}
	}

	// Returns true if there are free slots for additional fighters to go in
	bool CanAlloc()
	{
		return (usedCount<maxFighters);
	}

	void NewFighter(Simulation *sim, int fighterID, int i, int elem);

	void Simulation_Cleared(Simulation *sim) override
	{
		memset(fighters, 0, sizeof(fighters));
		usedCount = 0;
	}

	int MaxFighters()
	{
		return maxFighters;
	}
};

#endif
