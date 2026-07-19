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
#include "simulation/ToolNumbers.h"
#include "simulation/Tool.h"
#include "simulation/elements/FIGH.h"
#include "simulation/elements/PRTI.h"
#include "simulation/elements/STKM.h"

#define INBOND(x, y) ((x)>=0 && (y)>=0 && (x)<XRES && (y)<YRES)

const float dt = 0.9f; // Delta time in square
const float rocketBootsHeadEffect = 0.35f;
const float rocketBootsFeetEffect = 0.15f;
const float rocketBootsHeadEffectV = 0.3f; // stronger acceleration vertically, to counteract gravity
const float rocketBootsFeetEffectV = 0.45f;

void STKM_ElementDataContainer::Simulation_AfterUpdate(Simulation *sim)
{
	//Setting an element for the stick man
	if (sim->elementCount[PT_STKM] <= 0)
	{
		STKM_default_element(sim, &player);
	}
	if (sim->elementCount[PT_STKM2] <= 0)
	{
		STKM_default_element(sim, &player2);
	}
}

void STKM_ElementDataContainer::NewStickman1(int i, int elem)
{
	InitLegs(&player, i);
	if (elem >= 0 && elem < PT_NUM)
		player.elem = elem;
	player.spwn = 1;
}

void STKM_ElementDataContainer::NewStickman2(int i, int elem)
{
	InitLegs(&player2, i);
	if (elem >= 0 && elem < PT_NUM)
		player2.elem = elem;
	player2.spwn = 1;
}

void STKM_ElementDataContainer::Die(Simulation *sim, Stickman *playerp, int i)
{
	if (playerp->fan)
	{
		int x = (int)(sim->parts[i].x + 0.5f) / CELL;
		int y = (int)(sim->parts[i].y + 0.5f) / CELL;
		sim->air->pv[y][x] += 64;
	}
	else
	{
		int x = (int)(sim->parts[i].x + 0.5f);
		int y = (int)(sim->parts[i].y + 0.5f);
		for (int r = -2; r <= 1; r++)
		{
			sim->part_create(-1, x + r, y - 2, playerp->elem);
			sim->part_create(-1, x + r + 1, y + 2, playerp->elem);
			sim->part_create(-1, x - 2, y + r + 1, playerp->elem);
			sim->part_create(-1, x + 2, y + r, playerp->elem);
		}
	}
	sim->part_kill(i); // Kill him
}

int STKM_ElementDataContainer::Run(Stickman *playerp, UPDATE_FUNC_ARGS)
{
	int t = parts[i].type;

	if (!playerp->fan && sim->IsElement(parts[i].ctype))
		STKM_set_element(sim, playerp, parts[i].ctype);
	playerp->frames++;

	// Temperature handling
	if (parts[i].temp<243)
		parts[i].life -= 1;
	if ((parts[i].temp<309.6f) && (parts[i].temp>=243))
		parts[i].temp += 1;

	// If his HP is less than 0 or there is very big wind...
	if (parts[i].life < 1 || (sim->air->pv[y/CELL][x/CELL] >= 4.5f && !playerp->fan))
	{
		Die(sim, playerp, i);
		return 1;
	}

	// Follow gravity
	float gvx = 0.0f, gvy = 0.0f;
	switch (sim->gravityMode)
	{
		default:
		case GRAV_VERTICAL:
			gvy = 1;
			break;
		case GRAV_OFF:
			gvy = gvx = 0.0f;
			break;
		case GRAV_RADIAL:
		{
			float gravd;
			gravd = 0.01f - hypotf((parts[i].x - XCNTR), (parts[i].y - YCNTR));
			gvx = ((float)(parts[i].x - XCNTR) / gravd);
			gvy = ((float)(parts[i].y - YCNTR) / gravd);
			break;
		}
		case GRAV_CUSTOM:
			gvx = sim->customGravityX;
			gvy = sim->customGravityY;
			break;
	}

	gvx += sim->grav->gravx[((int)parts[i].y/CELL)*(XRES/CELL) + ((int)parts[i].x/CELL)];
	gvy += sim->grav->gravy[((int)parts[i].y/CELL)*(XRES/CELL) + ((int)parts[i].x/CELL)];

	float mvx = gvx;
	float mvy = gvy;
	bool rbLowGrav = false;
	float tmp = (fabsf(mvx) > fabsf(mvy)) ? fabsf(mvx) : fabsf(mvy);
	if (tmp < 0.001f)
	{
		rbLowGrav = true;
		mvx = -parts[i].vx;
		mvy = -parts[i].vy;
		tmp = fabsf(mvx) > fabsf(mvy)?fabsf(mvx):fabsf(mvy);
	}
	if (tmp < 0.001f)
	{
		mvx = 0;
		mvy = 1.0f;
		tmp = 1.0f;
	}
	tmp = 1.0f/sqrtf(mvx*mvx+mvy*mvy);
	mvx *= tmp; // scale to a unit vector
	mvy *= tmp;

	parts[i].vx -= gvx*dt; //Head up!
	parts[i].vy -= gvy*dt;

	// Verlet integration
	float pp = 2*playerp->legs[0]-playerp->legs[2]+playerp->accs[0]*dt*dt;
	playerp->legs[2] = playerp->legs[0];
	playerp->legs[0] = pp;
	pp = 2*playerp->legs[1]-playerp->legs[3]+playerp->accs[1]*dt*dt;
	playerp->legs[3] = playerp->legs[1];
	playerp->legs[1] = pp;

	pp = 2*playerp->legs[4]-playerp->legs[6]+(playerp->accs[2]+gvx)*dt*dt;
	playerp->legs[6] = playerp->legs[4];
	playerp->legs[4] = pp;
	pp = 2*playerp->legs[5]-playerp->legs[7]+(playerp->accs[3]+gvy)*dt*dt;
	playerp->legs[7] = playerp->legs[5];
	playerp->legs[5] = pp;

	pp = 2*playerp->legs[8]-playerp->legs[10]+playerp->accs[4]*dt*dt;
	playerp->legs[10] = playerp->legs[8];
	playerp->legs[8] = pp;
	pp = 2*playerp->legs[9]-playerp->legs[11]+playerp->accs[5]*dt*dt;
	playerp->legs[11] = playerp->legs[9];
	playerp->legs[9] = pp;

	pp = 2*playerp->legs[12]-playerp->legs[14]+(playerp->accs[6]+gvx)*dt*dt;
	playerp->legs[14] = playerp->legs[12];
	playerp->legs[12] = pp;
	pp = 2*playerp->legs[13]-playerp->legs[15]+(playerp->accs[7]+gvy)*dt*dt;
	playerp->legs[15] = playerp->legs[13];
	playerp->legs[13] = pp;

	// Setting acceleration to 0
	playerp->accs[0] = 0;
	playerp->accs[1] = 0;

	playerp->accs[2] = 0;
	playerp->accs[3] = 0;

	playerp->accs[4] = 0;
	playerp->accs[5] = 0;

	playerp->accs[6] = 0;
	playerp->accs[7] = 0;

	float gx = (playerp->legs[4] + playerp->legs[12])/2 - gvy;
	float gy = (playerp->legs[5] + playerp->legs[13])/2 + gvx;
	float dl = pow(gx - playerp->legs[4], 2) + pow(gy - playerp->legs[5], 2);
	float dr = pow(gx - playerp->legs[12], 2) + pow(gy - playerp->legs[13], 2);
	
	// Go left
	if (((int)(playerp->comm)&0x01) == 0x01)
	{
		bool moved = false;
		if (dl > dr)
		{
			if (INBOND(playerp->legs[4], playerp->legs[5]) && !sim->EvalMove(t, (int)playerp->legs[4], (int)playerp->legs[5]))
			{
				playerp->accs[2] = -3*mvy-3*mvx;
				playerp->accs[3] = 3*mvx-3*mvy;
				playerp->accs[0] = -mvy;
				playerp->accs[1] = mvx;
				moved = true;
			}
		}
		else
		{
			if (INBOND(playerp->legs[12], playerp->legs[13]) && !sim->EvalMove(t, (int)playerp->legs[12], (int)playerp->legs[13]))
			{
				playerp->accs[6] = -3*mvy-3*mvx;
				playerp->accs[7] = 3*mvx-3*mvy;
				playerp->accs[0] = -mvy;
				playerp->accs[1] = mvx;
				moved = true;
			}
		}
		if (!moved && playerp->rocketBoots)
		{
			parts[i].vx -= rocketBootsHeadEffect*mvy;
			parts[i].vy += rocketBootsHeadEffect*mvx;
			playerp->accs[2] -= rocketBootsFeetEffect*mvy;
			playerp->accs[6] -= rocketBootsFeetEffect*mvy;
			playerp->accs[3] += rocketBootsFeetEffect*mvx;
			playerp->accs[7] += rocketBootsFeetEffect*mvx;
			for (int leg = 0; leg < 2; leg++)
			{
				if (leg == 1 && (((int)(playerp->comm)&0x02) == 0x02))
					continue;
				int footX = (int)playerp->legs[leg*8+4], footY = (int)playerp->legs[leg*8+5];
				int np = sim->part_create(-1, footX, footY, PT_PLSM);
				if (np >= 0)
				{
					parts[np].vx = parts[i].vx+mvy*25;
					parts[np].vy = parts[i].vy-mvx*25;
					parts[np].life += 30;
				}
			}
		}
	}

	// Go right
	if (((int)(playerp->comm)&0x02) == 0x02)
	{
		bool moved = false;
		if (dl < dr)
		{
			if (INBOND(playerp->legs[4], playerp->legs[5]) && !sim->EvalMove(t, (int)playerp->legs[4], (int)playerp->legs[5]))
			{
				playerp->accs[2] = 3*mvy-3*mvx;
				playerp->accs[3] = -3*mvx-3*mvy;
				playerp->accs[0] = mvy;
				playerp->accs[1] = -mvx;
				moved = true;
			}
		}
		else
		{
			if (INBOND(playerp->legs[12], playerp->legs[13]) && !sim->EvalMove(t, (int)playerp->legs[12], (int)playerp->legs[13]))
			{
				playerp->accs[6] = 3*mvy-3*mvx;
				playerp->accs[7] = -3*mvx-3*mvy;
				playerp->accs[0] = mvy;
				playerp->accs[1] = -mvx;
				moved = true;
			}
		}
		if (!moved && playerp->rocketBoots)
		{
			parts[i].vx += rocketBootsHeadEffect*mvy;
			parts[i].vy -= rocketBootsHeadEffect*mvx;
			playerp->accs[2] += rocketBootsFeetEffect*mvy;
			playerp->accs[6] += rocketBootsFeetEffect*mvy;
			playerp->accs[3] -= rocketBootsFeetEffect*mvx;
			playerp->accs[7] -= rocketBootsFeetEffect*mvx;
			for (int leg = 0; leg < 2; leg++)
			{
				if (leg == 0 && (((int)(playerp->comm)&0x01) == 0x01))
					continue;
				int footX = (int)playerp->legs[leg*8+4], footY = (int)playerp->legs[leg*8+5];
				int np = sim->part_create(-1, footX, footY, PT_PLSM);
				if (np >= 0)
				{
					parts[np].vx = parts[i].vx-mvy*25;
					parts[np].vy = parts[i].vy+mvx*25;
					parts[np].life += 30;
				}
			}
		}
	}

	if (playerp->rocketBoots && ((int)(playerp->comm)&0x03) == 0x03)
	{
		// Pressing left and right simultaneously with rocket boots on slows the stickman down
		// Particularly useful in zero gravity
		parts[i].vx *= 0.5f;
		parts[i].vy *= 0.5f;
		playerp->accs[2] = playerp->accs[6] = 0;
		playerp->accs[3] = playerp->accs[7] = 0;
	}

	//Jump
	if (((int)(playerp->comm)&0x04) == 0x04)
	{
		if (playerp->rocketBoots)
		{
			if (rbLowGrav)
			{
				parts[i].vx -= rocketBootsHeadEffect*mvx;
				parts[i].vy -= rocketBootsHeadEffect*mvy;
				playerp->accs[2] -= rocketBootsFeetEffect*mvx;
				playerp->accs[6] -= rocketBootsFeetEffect*mvx;
				playerp->accs[3] -= rocketBootsFeetEffect*mvy;
				playerp->accs[7] -= rocketBootsFeetEffect*mvy;
			}
			else
			{
				parts[i].vx -= rocketBootsHeadEffectV*mvx;
				parts[i].vy -= rocketBootsHeadEffectV*mvy;
				playerp->accs[2] -= rocketBootsFeetEffectV*mvx;
				playerp->accs[6] -= rocketBootsFeetEffectV*mvx;
				playerp->accs[3] -= rocketBootsFeetEffectV*mvy;
				playerp->accs[7] -= rocketBootsFeetEffectV*mvy;
			}
			for (int leg = 0; leg < 2; leg++)
			{
				int footX = (int)playerp->legs[leg*8+4], footY = (int)playerp->legs[leg*8+5];
				int np = sim->part_create(-1, footX, footY+1, PT_PLSM);
				if (np >= 0)
				{
					parts[np].vx = parts[i].vx+mvx*30;
					parts[np].vy = parts[i].vy+mvy*30;
					parts[np].life += 10;
				}
			}
		}
		else if ((INBOND(playerp->legs[4], playerp->legs[5]) && !sim->EvalMove(t, (int)playerp->legs[4], (int)playerp->legs[5])) ||
				 (INBOND(playerp->legs[12], playerp->legs[13]) && !sim->EvalMove(t, (int)playerp->legs[12], (int)playerp->legs[13])))
		{
			parts[i].vx -= 4*mvx;
			parts[i].vy -= 4*mvy;
			playerp->accs[2] -= mvx;
			playerp->accs[6] -= mvx;
			playerp->accs[3] -= mvy;
			playerp->accs[7] -= mvy;
		}
	}

	// Charge detector wall if foot inside
	if (INBOND((int)(playerp->legs[4]+0.5), (int)(playerp->legs[5]+0.5)) &&
	        bmap[(int)(playerp->legs[5]+0.5)/CELL][(int)(playerp->legs[4]+0.5)/CELL]==WL_DETECT)
		set_emap((int)playerp->legs[4]/CELL, (int)playerp->legs[5]/CELL);
	if (INBOND((int)(playerp->legs[12]+0.5), (int)(playerp->legs[13]+0.5)) &&
	        bmap[(int)(playerp->legs[13]+0.5)/CELL][(int)(playerp->legs[12]+0.5)/CELL]==WL_DETECT)
		set_emap((int)(playerp->legs[12]+0.5)/CELL, (int)(playerp->legs[13]+0.5)/CELL);

	// Searching for particles near head
	for (int rx = -2; rx < 3; rx++)
		for (int ry = -2; ry < 3; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					r = photons[y+ry][x+rx];

				if (!r && !bmap[(y+ry)/CELL][(x+rx)/CELL])
					continue;

				STKM_set_element(sim, playerp, TYP(r));
				if (TYP(r) == PT_PLNT && parts[i].life<100) //Plant gives him 5 HP
				{
					if (parts[i].life<=95)
						parts[i].life += 5;
					else
						parts[i].life = 100;
					sim->part_kill(ID(r));
				}

				if (TYP(r) == PT_NEUT)
				{
					if (parts[i].life<=100) parts[i].life -= (102-parts[i].life)/2;
					else parts[i].life = (int)(parts[i].life*0.9f);
					sim->part_kill(ID(r));
				}
				if (bmap[(ry+y)/CELL][(rx+x)/CELL] == WL_FAN)
					playerp->fan = true;
				else if (bmap[(ry+y)/CELL][(rx+x)/CELL] == WL_EHOLE)
					playerp->rocketBoots = false;
				else if (bmap[(ry+y)/CELL][(rx+x)/CELL] == WL_GRAV)
					playerp->rocketBoots = true;
#ifdef NOMOD
				if (TYP(r) == PT_PRTI)
#else
				if (TYP(r) == PT_PRTI || TYP(r) == PT_PPTI)
#endif
					Interact(sim, playerp, i, rx, ry);

				// Interact() may kill STKM
				if (!parts[i].type)
					return 1;
			}

	// Head position
	int rx = x + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01);
	int ry = y - 3*(playerp->pcomm == 0);

	// Spawn
	if (((int)(playerp->comm)&0x08) == 0x08)
	{
		ry -= 2 * RNG::Ref().between(1, 2);
		int r = pmap[ry][rx];
		if (sim->elements[TYP(r)].Properties&TYPE_SOLID)
		{
			if (pmap[ry][rx])
				sim->spark_conductive_attempt(ID(pmap[ry][rx]), rx, ry);
			playerp->frames = 0;
		}
		else
		{
			int np = -1;
			if (playerp->fan)
			{
				for(int j = -4; j < 5; j++)
					for (int k = -4; k < 5; k++)
					{
						int x = rx + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01)+j;
						int y = ry+k;
						sim->air->pv[y/CELL][x/CELL] += 0.03f;
						if (y+CELL<YRES)
							sim->air->pv[y/CELL+1][x/CELL] += 0.03f;
						if (x+CELL<XRES)
						{
							sim->air->pv[y/CELL][x/CELL+1] += 0.03f;
							if (y+CELL<YRES)
								sim->air->pv[y/CELL+1][x/CELL+1] += 0.03f;
						}
					}
			}
			// limit lightning creation rate
			else if (playerp->elem==PT_LIGH && playerp->frames<30)
				np = -1;
			else
				np = sim->part_create(-1, rx, ry, playerp->elem);
			if ((np < NPART) && np >= 0)
			{
				if (playerp->elem == PT_PHOT)
				{
					int random = abs(RNG::Ref().between(-1, 1)) * 3;
					if (random == 0)
					{
						sim->part_kill(np);
					}
					else
					{
						parts[np].vy = 0;
						if (((int)playerp->pcomm)&(0x01|0x02))
							parts[np].vx = (float)(((((int)playerp->pcomm)&0x02) == 0x02) - (((int)(playerp->pcomm)&0x01) == 0x01))*random;
						else
							parts[np].vx = (float)random;
					}
				}
				else if (playerp->elem == PT_LIGH)
				{
					float angle;
					int power = 100;
					if (mvx != 0 || mvy != 0)
						angle = atan2(mvx, mvy)*180.0f/M_PI;
					else
						angle = RNG::Ref().between(0, 359);
					if (((int)playerp->pcomm)&0x01)
						angle += 180;
					if (angle > 360)
						angle -= 360;
					if (angle < 0)
						angle += 360;
					parts[np].tmp = (int)angle;
					parts[np].life = RNG::Ref().between(0, 1 + power / 15) + power / 7;
					parts[np].temp = parts[np].life * power/2.5f;
					parts[np].tmp2 = 1;
				}
				else if (!playerp->fan)
				{
					parts[np].vx -= -mvy*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01));
					parts[np].vy -= mvx*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01));
					parts[i].vx -= (sim->elements[(int)playerp->elem].Weight*parts[np].vx)/1000;
				}
				playerp->frames = 0;
			}

		}
	}

	// Simulation of joints
	float d = 25/(pow((playerp->legs[0]-playerp->legs[4]), 2) + pow((playerp->legs[1]-playerp->legs[5]), 2)+25) - 0.5f;  //Fast distance
	playerp->legs[4] -= (playerp->legs[0]-playerp->legs[4])*d;
	playerp->legs[5] -= (playerp->legs[1]-playerp->legs[5])*d;
	playerp->legs[0] += (playerp->legs[0]-playerp->legs[4])*d;
	playerp->legs[1] += (playerp->legs[1]-playerp->legs[5])*d;

	d = 25/(pow((playerp->legs[8]-playerp->legs[12]), 2) + pow((playerp->legs[9]-playerp->legs[13]), 2)+25) - 0.5f;
	playerp->legs[12] -= (playerp->legs[8]-playerp->legs[12])*d;
	playerp->legs[13] -= (playerp->legs[9]-playerp->legs[13])*d;
	playerp->legs[8] += (playerp->legs[8]-playerp->legs[12])*d;
	playerp->legs[9] += (playerp->legs[9]-playerp->legs[13])*d;

	d = 36/(pow((playerp->legs[0]-parts[i].x), 2) + pow((playerp->legs[1]-parts[i].y), 2)+36) - 0.5f;
	parts[i].vx -= (playerp->legs[0]-parts[i].x)*d;
	parts[i].vy -= (playerp->legs[1]-parts[i].y)*d;
	playerp->legs[0] += (playerp->legs[0]-parts[i].x)*d;
	playerp->legs[1] += (playerp->legs[1]-parts[i].y)*d;

	d = 36/(pow((playerp->legs[8]-parts[i].x), 2) + pow((playerp->legs[9]-parts[i].y), 2)+36) - 0.5f;
	parts[i].vx -= (playerp->legs[8]-parts[i].x)*d;
	parts[i].vy -= (playerp->legs[9]-parts[i].y)*d;
	playerp->legs[8] += (playerp->legs[8]-parts[i].x)*d;
	playerp->legs[9] += (playerp->legs[9]-parts[i].y)*d;

	if (INBOND(playerp->legs[4], playerp->legs[5]) && !sim->EvalMove(t, (int)playerp->legs[4], (int)playerp->legs[5]))
	{
		playerp->legs[4] = playerp->legs[6];
		playerp->legs[5] = playerp->legs[7];
	}

	if (INBOND(playerp->legs[12], playerp->legs[13]) && !sim->EvalMove(t, (int)playerp->legs[12], (int)playerp->legs[13]))
	{
		playerp->legs[12] = playerp->legs[14];
		playerp->legs[13] = playerp->legs[15];
	}

	//This makes stick man "pop" from obstacles
	if (INBOND(playerp->legs[4], playerp->legs[5]) && !sim->EvalMove(t, (int)playerp->legs[4], (int)playerp->legs[5]))
	{
		float t;
		t = playerp->legs[4]; playerp->legs[4] = playerp->legs[6]; playerp->legs[6] = t;
		t = playerp->legs[5]; playerp->legs[5] = playerp->legs[7]; playerp->legs[7] = t;
	}

	if (INBOND(playerp->legs[12], playerp->legs[13]) && !sim->EvalMove(t, (int)playerp->legs[12], (int)playerp->legs[13]))
	{
		float t;
		t = playerp->legs[12]; playerp->legs[12] = playerp->legs[14]; playerp->legs[14] = t;
		t = playerp->legs[13]; playerp->legs[13] = playerp->legs[15]; playerp->legs[15] = t;
	}

	// Keeping legs distance
	if ((pow((playerp->legs[4] - playerp->legs[12]), 2) + pow((playerp->legs[5]-playerp->legs[13]), 2)) < 16)
	{
		float tvx = -gvy;
		float tvy = gvx;

		if (tvx || tvy)
		{
			playerp->accs[2] -= 0.2f*tvx/hypot(tvx, tvy);
			playerp->accs[3] -= 0.2f*tvy/hypot(tvx, tvy);

			playerp->accs[6] += 0.2f*tvx/hypot(tvx, tvy);
			playerp->accs[7] += 0.2f*tvy/hypot(tvx, tvy);
		}
	}

	if ((pow((playerp->legs[0] - playerp->legs[8]), 2) + pow((playerp->legs[1]-playerp->legs[9]), 2)) < 16)
	{
		float tvx = -gvy;
		float tvy = gvx;

		if (tvx || tvy)
		{
			playerp->accs[0] -= 0.2f*tvx/hypot(tvx, tvy);
			playerp->accs[1] -= 0.2f*tvy/hypot(tvx, tvy);

			playerp->accs[4] += 0.2f*tvx/hypot(tvx, tvy);
			playerp->accs[5] += 0.2f*tvy/hypot(tvx, tvy);
		}
	}

	// If legs touch something
	Interact(sim, playerp, i, (int)(playerp->legs[4]+0.5), (int)(playerp->legs[5]+0.5));
	Interact(sim, playerp, i, (int)(playerp->legs[12]+0.5), (int)(playerp->legs[13]+0.5));
	Interact(sim, playerp, i, (int)(playerp->legs[4]+0.5), (int)playerp->legs[5]);
	Interact(sim, playerp, i, (int)(playerp->legs[12]+0.5), (int)playerp->legs[13]);
	if (!parts[i].type)
		return 1;

	parts[i].ctype = playerp->elem;
	return 0;
}

void STKM_ElementDataContainer::Interact(Simulation* sim, Stickman *playerp, int i, int x, int y)
{
	if (x<0 || y<0 || x>=XRES || y>=YRES || !parts[i].type)
		return;
	int r = pmap[y][x];
	if (r)
	{
		int damage = 0;
		if (TYP(r)==PT_SPRK && playerp->elem!=PT_LIGH) //If on charge
		{
			damage += RNG::Ref().between(32, 51);
		}

		if (!sim->IsHeatInsulator(sim->parts[ID(r)]) && ((playerp->elem!=PT_LIGH && parts[ID(r)].temp>=323) || parts[ID(r)].temp<=243) && (!playerp->rocketBoots || TYP(r)!=PT_PLSM))
		{
			damage += 2;
			playerp->accs[3] -= 1;
		}

		if (sim->elements[TYP(r)].Properties&PROP_DEADLY)
			switch TYP(r)
			{
				case PT_ACID:
					damage += 5;
					break;
				default:
					damage++;
			}

		if (sim->elements[TYP(r)].Properties&PROP_RADIOACTIVE)
			damage++;

		if (damage)
		{
			if (damage > sim->parts[i].life)
			{
				Die(sim, playerp, i);
				return;
			}
			sim->parts[i].life -= damage;
		}
#ifdef NOMOD
		if (TYP(r)==PT_PRTI && parts[i].type)
#else
		if ((TYP(r)==PT_PRTI || TYP(r)==PT_PPTI) && parts[i].type)
#endif
		{
			int t = parts[i].type;
			unsigned char tmp = TYP(parts[i].tmp);
			PortalChannel *channel = static_cast<PRTI_ElementDataContainer&>(*sim->elementData[PT_PRTI]).GetParticleChannel(sim, ID(r));
			if (channel->StoreParticle(sim, i, 1))//slot=1 gives rx=0, ry=1 in PRTO_update
			{
				//stop new STKM/fighters being created to replace the ones in the portal:
				if (t==PT_FIGH)
					static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).AllocSpecific(tmp);
				else
					playerp->spwn = 1;
			}
		}

		if ((TYP(r)==PT_BHOL || TYP(r)==PT_NBHL) && parts[i].type)
		{
			if (!legacy_enable)
			{
				parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp+parts[i].temp/2, MIN_TEMP, MAX_TEMP);
			}
			sim->part_kill(i);
		}
		if ((TYP(r)==PT_VOID || (TYP(r)==PT_PVOD && parts[ID(r)].life==10)) && (!parts[ID(r)].ctype || (parts[ID(r)].ctype==parts[i].type)!=(parts[ID(r)].tmp&1)) && parts[i].type)
		{
			sim->part_kill(i);
		}
	}
}

void STKM_ElementDataContainer::HandleKeyPress(StkmKeys key, bool stk2)
{
	//  4
	//1 8 2
	Stickman *movedPlayer = stk2 ? &player2 : &player;
	if (key == Left)
		movedPlayer->comm = (int)(movedPlayer->comm) | 0x01;
	else if (key == Right)
		movedPlayer->comm = (int)(movedPlayer->comm) | 0x02;
	else if (key == Up && ((int)(player.comm) & 0x04) != 0x04)
		movedPlayer->comm = (int)(movedPlayer->comm) | 0x04;
	else if (key == Down && ((int)(player.comm) & 0x08) != 0x08)
		movedPlayer->comm = (int)(movedPlayer->comm) | 0x08;
}

void STKM_ElementDataContainer::HandleKeyRelease(StkmKeys key, bool stk2)
{
	Stickman *movedPlayer = stk2 ? &player2 : &player;
	if (key == Left || key == Right)
	{
		// Saving last movement
		movedPlayer->pcomm = movedPlayer->comm;
		movedPlayer->comm = (int)(movedPlayer->comm) & ~(0x1 | 0x2);
	}
	else if (key == Up)
		movedPlayer->comm = (int)(movedPlayer->comm) & ~0x04;
	else if (key == Down)
		movedPlayer->comm = (int)(movedPlayer->comm) & ~0x08;
}

void STKM_ElementDataContainer::STKM_default_element(Simulation *sim, Stickman *playerp)
{
	int sr = ((ElementTool*)activeTools[1])->GetID();
	if (sim->IsElement(sr))
		STKM_set_element(sim, playerp, sr);
	else
		playerp->elem = PT_DUST;
}

void STKM_ElementDataContainer::STKM_set_element(Simulation *sim, Stickman *playerp, int element)
{
	if (sim->elements[element].Falldown != 0
	    || sim->elements[element].Properties&TYPE_GAS
	    || sim->elements[element].Properties&TYPE_LIQUID
	    || sim->elements[element].Properties&TYPE_ENERGY
	    || element == PT_LOLZ || element == PT_LOVE)
	{
		if (!playerp->rocketBoots || element != PT_PLSM)
		{
			playerp->elem = element;
			playerp->fan = false;
		}
	}
	if (element == PT_TESC || element == PT_LIGH)
	{
		playerp->elem = PT_LIGH;
		playerp->fan = false;
	}
}

int STKM_update(UPDATE_FUNC_ARGS)
{
	static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).Run(static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1(), UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

int STKM_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = *colg = *colb = *cola = 0;
	*pixel_mode = PSPEC_STICKMAN;
	return 0;
}

bool STKM_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return sim->elementCount[PT_STKM]<=0 && !static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1()->spwn;
}

void STKM_create(ELEMENT_CREATE_FUNC_ARGS)
{
	int id = sim->part_create(-3, x, y, PT_SPAWN);
	if (id >= 0)
		static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1()->spawnID = id;
}

void STKM_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to == PT_STKM)
		static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).NewStickman1(i, -1);
	else
		static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1()->spwn = 0;
}

void STKM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STKM";
	elem->Name = "STKM";
	elem->Colour = COLPACK(0xFFE0A0);
#ifndef TOUCHUI
	elem->MenuVisible = 1;
#else
	elem->MenuVisible = 0;
#endif
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.2f;
	elem->Loss = 1.0f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->NewtonianGravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->HotAir = 0.00f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 50;

	elem->DefaultProperties.temp = R_TEMP + 14.6f + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Stickman. Don't kill him! Control with the arrow keys.";

	elem->Properties = PROP_NOCTYPEDRAW;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 620.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->DefaultProperties.life = 100;

	elem->Update = &STKM_update;
	elem->Graphics = &STKM_graphics;
	elem->Func_Create_Allowed = &STKM_create_allowed;
	elem->Func_Create = &STKM_create;
	elem->Func_ChangeType = &STKM_ChangeType;
	elem->Init = &STKM_init_element;

	sim->elementData[t].reset(new STKM_ElementDataContainer);
}
