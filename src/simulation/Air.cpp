/**
 * Powder Toy - air simulation
 *
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

#include <cmath>
#include <cstring> // memcpy
#include "simulation/Air.h"
#include "defines.h"
#include "simulation/Simulation.h"
#include "simulation/WallNumbers.h"

Air::Air()
{
	MakeKernel();
	ambientAirTemp = R_TEMP + 273.15;
	ambientAirTempPref = R_TEMP + 273.15;

	Clear();
}

// Kernel, used for velocity
void Air::MakeKernel()
{
	float s = 0.0f;
	for (int j = -1; j <= 1; j++)
		for (int i = -1; i <= 1; i++)
		{
			kernel[(i+1)+3*(j+1)] = expf(-2.0f*(i*i+j*j));
			s += kernel[(i+1)+3*(j+1)];
		}
	s = 1.0f / s;
	for (int j = -1; j <= 1; j++)
		for (int i = -1; i <= 1; i++)
			kernel[(i+1)+3*(j+1)] *= s;
}

void Air::Clear()
{
	std::fill(&pv[0][0], &pv[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&vy[0][0], &vy[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&vx[0][0], &vx[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&fvy[0][0], &fvy[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&fvx[0][0], &fvx[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&blockair[0][0], &blockair[0][0]+((XRES/CELL)*(YRES/CELL)), 0);
	std::fill(&blockairh[0][0], &blockairh[0][0]+((XRES/CELL)*(YRES/CELL)), 0);

	float airTemp = GetAmbientAirTemp();
	for (int x = 0; x < XRES/CELL; x++)
	{
		for (int y = 0; y < YRES/CELL; y++)
		{
			hv[y][x] = airTemp;
		}
	}
}

void Air::ClearAirH()
{
	std::fill(&hv[0][0], &hv[0][0]+((XRES/CELL)*(YRES/CELL)), GetAmbientAirTemp());
}

void Air::UpdateAirHeat(Simulation *sim)
{
	if (!aheat_enable)
		return;

	float ambientAirTemp = GetAmbientAirTemp();

	// Set ambient heat temp on the edges every frame
	for (int i = 0; i < YRES/CELL; i++)
	{
		hv[i][0] = ambientAirTemp;
		hv[i][1] = ambientAirTemp;
		hv[i][XRES/CELL-2] = ambientAirTemp;
		hv[i][XRES/CELL-1] = ambientAirTemp;
	}

	// Set ambient heat temp on the edges every frame
	for (int i = 0; i < XRES/CELL; i++)
	{
		hv[0][i] = ambientAirTemp;
		hv[1][i] = ambientAirTemp;
		hv[YRES/CELL-2][i] = ambientAirTemp;
		hv[YRES/CELL-1][i] = ambientAirTemp;
	}

	float dh, dx, dy;
	float f;
	float txf, tyf;
	int txi, tyi;
	// Update ambient heat
	for (int y = 0; y < YRES/CELL; y++)
	{
		for (int x = 0; x < XRES/CELL; x++)
		{
			dh = 0.0f;
			dx = 0.0f;
			dy = 0.0f;
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (y+j > 0 && y+j < YRES/CELL-2 && x+i > 0 && x+i < XRES/CELL-2 &&
					        !(blockairh[y+j][x+i]&0x8))
					{
						f = kernel[i+1+(j+1)*3];
						dh += hv[y+j][x+i]*f;
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
					}
					else
					{
						f = kernel[i+1+(j+1)*3];
						dh += hv[y][x]*f;
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
					}
				}
			}
			txf = x - dx*0.7f;
			tyf = y - dy*0.7f;
			txi = (int)txf;
			tyi = (int)tyf;
			txf -= txi;
			tyf -= tyi;
			if (txi >= 2 && txi < XRES/CELL-3 && tyi >= 2 && tyi < YRES/CELL-3)
			{
				float odh = dh;
				dh *= 1.0f - AIR_VADV;
				dh += AIR_VADV * (1.0f-txf) * (1.0f-tyf) * ((blockairh[tyi][txi]&0x8) ? odh : hv[tyi][txi]);
				dh += AIR_VADV * txf * (1.0f-tyf) * ((blockairh[tyi][txi+1]&0x8) ? odh : hv[tyi][txi+1]);
				dh += AIR_VADV * (1.0f-txf) * tyf * ((blockairh[tyi+1][txi]&0x8) ? odh : hv[tyi+1][txi]);
				dh += AIR_VADV * txf * tyf * ((blockairh[tyi+1][txi+1]&0x8) ? odh : hv[tyi+1][txi+1]);
			}
			pv[y][x] += (dh - hv[y][x]) / 5000.0f;

			if (x>=2 && x<XRES/CELL-2 && y>=2 && y<YRES/CELL-2)
			{
				float convGravX, convGravY;
				sim->GetGravityField(x*CELL, y*CELL, -1.0f, -1.0f, convGravX, convGravY);
				auto weight = ((hv[y][x] - hv[y][x-1]) * convGravX + (hv[y][x] - hv[y-1][x]) * convGravY) / 5000.0f;
				if (weight > 0 && !(blockairh[y-1][x]&0x8))
				{
					vx[y][x] += weight * convGravX;
					vy[y][x] += weight * convGravY;
				}
			}
			ohv[y][x] = dh;
		}
	}
	memcpy(hv, ohv, sizeof(hv));
}

void Air::UpdateAir()
{
	// "No Update"
	if (airMode == 4)
		return;

	// Reduces pressure/velocity on the edges every frame
	for (int i = 0; i < YRES/CELL; i++)
	{
		pv[i][0] = pv[i][0]*0.8f;
		pv[i][1] = pv[i][1]*0.8f;
		pv[i][XRES/CELL-2] = pv[i][XRES/CELL-2]*0.8f;
		pv[i][XRES/CELL-1] = pv[i][XRES/CELL-1]*0.8f;
		vx[i][0] = vx[i][0]*0.9f;
		vx[i][1] = vx[i][1]*0.9f;
		vx[i][XRES/CELL-2] = vx[i][XRES/CELL-2]*0.9f;
		vx[i][XRES/CELL-1] = vx[i][XRES/CELL-1]*0.9f;
		vy[i][0] = vy[i][0]*0.9f;
		vy[i][1] = vy[i][1]*0.9f;
		vy[i][XRES/CELL-2] = vy[i][XRES/CELL-2]*0.9f;
		vy[i][XRES/CELL-1] = vy[i][XRES/CELL-1]*0.9f;
	}

	// Reduces pressure/velocity on the edges every frame
	for (int i = 0; i < XRES/CELL; i++)
	{
		pv[0][i] = pv[0][i]*0.8f;
		pv[1][i] = pv[1][i]*0.8f;
		pv[YRES/CELL-2][i] = pv[YRES/CELL-2][i]*0.8f;
		pv[YRES/CELL-1][i] = pv[YRES/CELL-1][i]*0.8f;
		vx[0][i] = vx[0][i]*0.9f;
		vx[1][i] = vx[1][i]*0.9f;
		vx[YRES/CELL-2][i] = vx[YRES/CELL-2][i]*0.9f;
		vx[YRES/CELL-1][i] = vx[YRES/CELL-1][i]*0.9f;
		vy[0][i] = vy[0][i]*0.9f;
		vy[1][i] = vy[1][i]*0.9f;
		vy[YRES/CELL-2][i] = vy[YRES/CELL-2][i]*0.9f;
		vy[YRES/CELL-1][i] = vy[YRES/CELL-1][i]*0.9f;
	}

	// Clear some velocities near walls
	for (int j = 1; j < YRES / CELL - 1; j++)
	{
		for (int i = 1; i < XRES / CELL - 1; i++)
		{
			if (blockair[j][i])
			{
				vx[j][i] = 0.0f;
				vx[j][i-1] = 0.0f;
				vx[j][i+1] = 0.0f;
				vy[j][i] = 0.0f;
				vy[j-1][i] = 0.0f;
				vy[j+1][i] = 0.0f;
			}
		}
	}

	// Pressure adjustments from velocity
	for (int y = 1; y < YRES / CELL - 1; y++)
		for (int x = 1; x < XRES / CELL - 1; x++)
		{
			float dp = (vx[y][x-1] - vx[y][x+1]) + (vy[y-1][x] - vy[y+1][x]);
			pv[y][x] *= AIR_PLOSS;
			pv[y][x] += dp * AIR_TSTEPP * 0.5f;
		}

	// Velocity adjustments from pressure
	for (int y = 1; y < YRES/CELL-1; y++)
		for (int x = 1; x < XRES/CELL-1; x++)
		{
			float dx = pv[y][x-1] - pv[y][x+1];
			float dy = pv[y-1][x] - pv[y+1][x];
			vx[y][x] *= AIR_VLOSS;
			vy[y][x] *= AIR_VLOSS;
			vx[y][x] += dx * AIR_TSTEPV * 0.5f;
			vy[y][x] += dy * AIR_TSTEPV * 0.5f;
			if (blockair[y][x-1] || blockair[y][x] || blockair[y][x+1])
				vx[y][x] = 0;
			if (blockair[y-1][x] || blockair[y][x] || blockair[y+1][x])
				vy[y][x] = 0;
		}

	const float advDistanceMult = 0.7f;
	float dp, dx, dy;
	float f;
	float txf, tyf;
	int txi, tyi;
	float stepX, stepY;
	int stepLimit, step;
	// Update velocity and pressure
	for (int y = 0; y < YRES/CELL; y++)
		for (int x = 0; x < XRES/CELL; x++)
		{
			dx = 0.0f;
			dy = 0.0f;
			dp = 0.0f;
			for (int j = -1; j <= 1; j++)
				for (int i = -1; i <= 1; i++)
					if (y+j>0 && y+j<YRES/CELL-1 &&
							x+i>0 && x+i<XRES/CELL-1 &&
							!blockair[y+j][x+i])
					{
						f = kernel[i+1+(j+1)*3];
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
						dp += pv[y+j][x+i]*f;
					}
					else
					{
						f = kernel[i+1+(j+1)*3];
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
						dp += pv[y][x]*f;
					}


			txf = x - dx * advDistanceMult;
			tyf = y - dy * advDistanceMult;
			if ((dx * advDistanceMult > 1.0f || dy * advDistanceMult > 1.0f) && (txf >= 2 && txf < XRES/CELL-2 && tyf >= 2 && tyf < YRES/CELL-2))
			{
				// Trying to take velocity from far away, check whether there is an intervening wall. Step from current position to desired source location, looking for walls, with either the x or y step size being 1 cell
				if (std::abs(dx) > std::abs(dy))
				{
					stepX = (dx < 0.0f) ? 1.0f : -1.0f;
					stepY = -dy / std::abs(dx);
					stepLimit = (int)(std::abs(dx * advDistanceMult));
				}
				else
				{
					stepY = (dy < 0.0f) ? 1.0f : -1.0f;
					stepX = -dx / std::abs(dy);
					stepLimit = (int)(std::abs(dy * advDistanceMult));
				}
				txf = (float)x;
				tyf = (float)y;
				for (step = 0; step < stepLimit; ++step)
				{
					txf += stepX;
					tyf += stepY;
					if (blockair[(int)(tyf+0.5f)][(int)(txf+0.5f)])
					{
						txf -= stepX;
						tyf -= stepY;
						break;
					}
				}
				if (step == stepLimit)
				{
					// No wall found
					txf = x - dx * advDistanceMult;
					tyf = y - dy * advDistanceMult;
				}
			}
			txi = (int)txf;
			tyi = (int)tyf;
			txf -= txi;
			tyf -= tyi;
			if (!blockair[y][x] && txi >= 2 && txi <= XRES/CELL-3 && tyi >= 2 && tyi <= YRES/CELL-3)
			{
				dx *= 1.0f - AIR_VADV;
				dy *= 1.0f - AIR_VADV;

				dx += AIR_VADV * (1.0f-txf) * (1.0f-tyf) * vx[tyi][txi];
				dy += AIR_VADV * (1.0f-txf) * (1.0f-tyf) * vy[tyi][txi];

				dx += AIR_VADV * txf * (1.0f-tyf) * vx[tyi][txi+1];
				dy += AIR_VADV * txf * (1.0f-tyf) * vy[tyi][txi+1];

				dx += AIR_VADV * (1.0f-txf) * tyf * vx[tyi+1][txi];
				dy += AIR_VADV * (1.0f-txf) * tyf * vy[tyi+1][txi];

				dx += AIR_VADV * txf * tyf * vx[tyi+1][txi+1];
				dy += AIR_VADV * txf * tyf * vy[tyi+1][txi+1];
			}

			if (bmap[y][x] == WL_FAN)
			{
				dx += fvx[y][x];
				dy += fvy[y][x];
			}

			// pressure/velocity caps
			if (dp > 256.0f)
				dp = 256.0f;
			else if (dp < -256.0f)
				dp = -256.0f;

			if (dx > 256.0f)
				dx = 256.0f;
			else if (dx < -256.0f)
				dx = -256.0f;

			if (dy > 256.0f)
				dy = 256.0f;
			else if (dy < -256.0f)
				dy = -256.0f;

			switch (airMode)
			{
			// Default
			default:
			case 0:
				break;
			// "Pressure off"
			case 1:
				dp = 0.0f;
				break;
			// "Velocity off"
			case 2:
				dx = 0.0f;
				dy = 0.0f;
				break;
			// "Off"
			case 3:
				dx = 0.0f;
				dy = 0.0f;
				dp = 0.0f;
				break;
			}

			ovx[y][x] = dx;
			ovy[y][x] = dy;
			opv[y][x] = dp;
		}
	memcpy(vx, ovx, sizeof(vx));
	memcpy(vy, ovy, sizeof(vy));
	memcpy(pv, opv, sizeof(pv));
}

// called when loading saves / stamps to ensure nothing "leaks" the first frame
// copied from tpt++
// turns out ... it was only a tpt++ bug. This mod updates air after the simulation, so TTAN sets wallmap blocking properly on save loads
// commented out for now, but left in for consistency in case air updates are moved later
void Air::RecalculateBlockAirMaps(Simulation * sim)
{
	/*for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
	{
		int type = sim->parts[i].type;
		if (!type)
			continue;
		// Real TTAN would only block if there was enough TTAN
		// but it would be more expensive and complicated to actually check that
		// so just block for a frame, if it wasn't supposed to block it will continue allowing air next frame
		if (type == PT_TTAN)
		{
			int x = ((int)(sim->parts[i].x+0.5f))/CELL, y = ((int)(sim->parts[i].y+0.5f))/CELL;
			if (sim->InBounds(x, y))
			{
				bmap_blockair[y][x] = 1;
				bmap_blockairh[y][x] = 0x8;
			}
		}
		// mostly accurate insulator blocking, besides checking GEL
		else if ((type == PT_HSWC && sim->parts[i].life != 10) || sim->elements[type].HeatConduct <= (rand()%250))
		{
			int x = ((int)(sim->parts[i].x+0.5f))/CELL, y = ((int)(sim->parts[i].y+0.5f))/CELL;
			if (sim->InBounds(x, y) && !(bmap_blockairh[y][x]&0x8))
				bmap_blockairh[y][x]++;
		}
	}*/
}

void Air::SetAmbientAirTemp(float ambientAirTemp)
{
	this->ambientAirTemp = ambientAirTemp;
}

void Air::SetAmbientAirTempPref(float ambientAirTemp)
{
	this->ambientAirTemp = ambientAirTemp;
	this->ambientAirTempPref = ambientAirTemp;
}

void Air::ClearTemporaryAirTemp()
{
	// Not called on clear_sim, to ensure correct ambient air temp is set when loading saves
	this->ambientAirTemp = this->ambientAirTempPref;
}

float Air::GetAmbientAirTemp()
{
	return ambientAirTemp;
}

float Air::GetAmbientAirTempPref()
{
	return ambientAirTempPref;
}
