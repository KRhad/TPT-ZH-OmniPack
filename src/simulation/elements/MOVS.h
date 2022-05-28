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

#ifndef SIMULATION_ELEMENTS_MOVS_H
#define SIMULATION_ELEMENTS_MOVS_H
#ifndef NOMOD

#include <cmath>
#include "simulation/ElementDataContainer.h"
#include "powder.h"
#include "simulation/Simulation.h"

#define MAX_MOVING_SOLIDS 256

void rotate(float *x, float *y, float angle);

class MovingSolid
{
public:
	float vx, vy;
	float rotationOld, rotation;
	int index;
	int particleCount; //number of particles attached to it

	void Simulation_Cleared()
	{
		vx = vy = 0.0f;
		rotationOld = rotation = 0.0f;
		index = 0;
		particleCount = 0;
	}
};

class MOVS_ElementDataContainer : public ElementDataContainer
{
private:
	MovingSolid movingSolids[MAX_MOVING_SOLIDS];
	int numBalls;

public:
	int creatingSolid;
	MOVS_ElementDataContainer():
		numBalls(0)
	{
		for (int i = 0; i < MAX_MOVING_SOLIDS; i++)
			movingSolids[i].Simulation_Cleared();
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<MOVS_ElementDataContainer>(*this); }

	MovingSolid* GetMovingSolid(int bn)
	{
		if (bn >= 0 && bn < MAX_MOVING_SOLIDS)
			return movingSolids + bn;
		return NULL;
	}

	int GetNumBalls()
	{
		return numBalls;
	}

	void SetNumBalls(int num)
	{
		numBalls = num;
	}

	void Simulation_Cleared(Simulation *sim) override
	{
		for (int i = 0; i < MAX_MOVING_SOLIDS; i++)
			movingSolids[i].Simulation_Cleared();
		numBalls = 0;
		creatingSolid = 0;
	}

	void CreateMovingSolidCenter(int i)
	{
		if (numBalls >= 255)
			return;

		parts[i].tmp2 = numBalls;
		parts[i].tmp3 = 0;
		parts[i].tmp4 = 0;
		MovingSolid *movingSolid = GetMovingSolid(numBalls++);
		if (movingSolid)
		{
			movingSolid->index = i+1;
			movingSolid->particleCount = 1;
			movingSolid->vx = 0;
			movingSolid->vy = 0;
			movingSolid->rotationOld = movingSolid->rotation = 0;
		}

		creatingSolid = numBalls;
	}

	void CreateMovingSolid(int i, int x, int y)
	{
		int bn = creatingSolid-1;
		MovingSolid *movingSolid = GetMovingSolid(bn);
		if (movingSolid && movingSolid->index)
		{
			parts[i].tmp2 = bn;
			parts[i].tmp3 = x - parts[movingSolid->index-1].x;
			parts[i].tmp4 = y - parts[movingSolid->index-1].y;
			movingSolid->particleCount++;
		}
	}

	bool IsCreatingSolid()
	{
		return creatingSolid != 0;
	}

	void Simulation_BeforeUpdate(Simulation *sim) override
	{
		creatingSolid = 0;
	}

	void Simulation_AfterUpdate(Simulation *sim) override
	{
		if (numBalls == 0)
			return;
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
		{
			if (parts[i].flags&FLAG_DISAPPEAR)
			{
				sim->part_kill(i);
			}
			else if (parts[i].type == PT_MOVS)
			{
				MovingSolid *movingSolid = GetMovingSolid(parts[i].tmp2);
				if (!movingSolid || !movingSolid->index)
					continue;

				movingSolid->vx = movingSolid->vx + parts[i].vx;
				movingSolid->vy = movingSolid->vy + parts[i].vy;
			}
		}
		for (int bn = 0; bn < numBalls; bn++)
		{
			MovingSolid *movingSolid = GetMovingSolid(bn);
			if (!movingSolid || !movingSolid->index)
				continue;

			movingSolid->vx = movingSolid->vx/movingSolid->particleCount;
			movingSolid->vy = movingSolid->vy/movingSolid->particleCount;
			switch (sim->gravityMode)
			{
			case 0:
				movingSolid->vy = movingSolid->vy + .2f;
				break;
			case 1:
				break;
			case 2:
				float pGravD = 0.01f - hypotf((parts[movingSolid->index-1].x - XCNTR), (parts[movingSolid->index-1].y - YCNTR));
				movingSolid->vx = movingSolid->vx + .2f * ((parts[movingSolid->index-1].x - XCNTR) / pGravD);
				movingSolid->vy = movingSolid->vy + .2f * ((parts[movingSolid->index-1].y - YCNTR) / pGravD);
				break;
			}
			movingSolid->rotationOld = movingSolid->rotation;
			if (!sim->msRotation)
			{
				movingSolid->rotationOld = 0;
			}
			else if (movingSolid->rotationOld > 2*M_PI)
			{
				movingSolid->rotationOld -= 2*M_PI;
			}
			else if (movingSolid->rotationOld < -2*M_PI)
			{
				movingSolid->rotationOld += 2*M_PI;
			}
		}
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
		{
			if (parts[i].type == PT_MOVS)
			{
				MovingSolid *movingSolid = GetMovingSolid(parts[i].tmp2);
				if (!movingSolid || (parts[i].flags&FLAG_DISAPPEAR))
					continue;
				if (movingSolid->index)
				{
					float tmp3 = parts[i].tmp3;
					float tmp4 = parts[i].tmp4;
					if (sim->msRotation)
						rotate(&tmp3, &tmp4, movingSolid->rotationOld);
					float nx = parts[movingSolid->index-1].x + tmp3;
					float ny = parts[movingSolid->index-1].y + tmp4;
					sim->Move(i,(int)(parts[i].x+.5f),(int)(parts[i].y+.5f),nx,ny);

					if (sim->msRotation)
					{
						rotate(&tmp3, &tmp4, .02f);
						if (parts[movingSolid->index-1].x + tmp3 != nx || parts[movingSolid->index-1].y + tmp4 != ny)
						{
							int j = sim->part_create(-1, (int)(parts[movingSolid->index-1].x + tmp3), (int)(parts[movingSolid->index-1].y + tmp4), parts[i].type);
							if (j >= 0)
							{
								parts[j].flags |= FLAG_DISAPPEAR;
								parts[j].tmp2 = parts[i].tmp2;
								parts[j].dcolour = parts[i].dcolour;
							}
						}
					}

					parts[i].vx = movingSolid->vx;
					parts[i].vy = movingSolid->vy;
				}
				if (sim->OutOfBounds((int)(parts[i].x+.5f), (int)(parts[i].y+.5f)))//kill_part if particle is out of bounds
					sim->part_kill(i);
			}
		}
		for (int bn = 0; bn < numBalls; bn++)
		{
			MovingSolid *movingSolid = GetMovingSolid(bn);
			movingSolid->vx = 0;
			movingSolid->vy = 0;
		}
	}
};

#endif
#endif
