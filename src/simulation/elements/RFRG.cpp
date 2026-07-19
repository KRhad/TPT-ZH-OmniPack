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

int RFRG_update(UPDATE_FUNC_ARGS)
{
	float new_pressure = sim->air->pv[y/CELL][x/CELL];
	float *old_pressure = (float *)&parts[i].tmp;
	if (std::isnan(*old_pressure))
	{
		*old_pressure = new_pressure;
		return 0;
	}
	
	// * 0 bar seems to be pressure value -256 in TPT, see Air.cpp. Also, 1 bar seems to be pressure value 0.
	//   With those two values we can set up our pressure scale which states that ... the highest pressure
	//   we can achieve in TPT is 2 bar. That's not particularly realistic, but good enough for TPT.
	
	parts[i].temp = restrict_flt(parts[i].temp * ((new_pressure + 257.f) / (*old_pressure + 257.f)), 0, MAX_TEMP);
	*old_pressure = new_pressure;
	return 0;
}

void RFRG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RFRG";
	elem->Name = "RFRG";
	elem->Colour = PIXPACK(0x72D2D4);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.2f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 1.3f;
	elem->HotAir = 0.0001f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 21;

	elem->Weight = 1;

	elem->HeatConduct = 3;
	elem->Description = "Refrigerant. Heats up and liquefies under pressure.";

	elem->Properties = TYPE_GAS|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 2;
	elem->HighPressureTransitionElement = PT_RFGL;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &RFRG_update;
}
