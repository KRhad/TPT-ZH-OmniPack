/**
 * Powder Toy - air simulation (header)
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

#ifndef AIR_H
#define AIR_H

#include "defines.h"

class Simulation;

class Air
{
	// used to calculate & store new air maps off of the old ones
	float opv[YRES/CELL][XRES/CELL];
	float ovx[YRES/CELL][XRES/CELL];
	float ovy[YRES/CELL][XRES/CELL];

	float ambientAirTemp;
	float ambientAirTempPref;

public:
	float pv[YRES/CELL][XRES/CELL];
	float vx[YRES/CELL][XRES/CELL];
	float vy[YRES/CELL][XRES/CELL];
	unsigned char bmap_blockair[YRES/CELL][XRES/CELL];
	unsigned char bmap_blockairh[YRES/CELL][XRES/CELL];

	// Fan velocity
	float fvx[YRES/CELL][XRES/CELL], fvy[YRES/CELL][XRES/CELL];

	// Ambient Heat
	float hv[YRES/CELL][XRES/CELL], ohv[YRES/CELL][XRES/CELL];

	float kernel[9];

	Air();
	void MakeKernel();
	
	void Clear();
	void ClearAirH();

	void UpdateAirHeat(Simulation * sim);
	void UpdateAir();

	void RecalculateBlockAirMaps(Simulation * sim);

	void SetAmbientAirTemp(float ambientAirTemp);
	void SetAmbientAirTempPref(float ambientAirTemp);
	void ClearTemporaryAirTemp();
	float GetAmbientAirTemp();
	float GetAmbientAirTempPref();
};

#endif
