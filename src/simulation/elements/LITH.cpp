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

/*

tmp2:  carbonation factor
life:  burn timer above 1000, spark cooldown timer otherwise
tmp:   hydrogenation factor
ctype: absorbed energy

For game reasons, baseline LITH has the reactions of both its pure form and
its hydroxide, and also has basic li-ion battery-like behavior.
It absorbs CO2 like its hydroxide form, but can only be converted into GLAS
after having absorbed CO2.

*/

int LITH_update(UPDATE_FUNC_ARGS)
{
	particle &self = parts[i];

	int &hydrogenationFactor = self.tmp;
	int &burnTimer = self.life;
	int &carbonationFactor = self.tmp2;
	int &storedEnergy = self.ctype;
	if (storedEnergy < 0)
	{
		storedEnergy = 0;
	}

	bool charged = false;
	bool discharged = false;
	for (int rx = -2; rx <= 2; ++rx)
	{
		for (int ry = -2; ry <= 2; ++ry)
		{
			if (BOUNDS_CHECK && (rx || ry))
			{
				int neighborData = pmap[y + ry][x + rx];
				if (!neighborData)
				{
					if (burnTimer > 1012 && RNG::Ref().chance(1, 10))
					{
						sim->part_create(-1, x + rx, y + ry, PT_FIRE);
					}
					continue;
				}
				particle &neighbor = parts[ID(neighborData)];
				
				switch (TYP(neighborData))
				{
					case PT_SLTW:
					case PT_WTRV:
					case PT_WATR:
					case PT_DSTW:
					case PT_CBNW:
						if (burnTimer > 1016)
						{
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_WTRV);
							neighbor.temp = 440.f;
							continue;
						}
						if (hydrogenationFactor + carbonationFactor >= 10)
						{
							continue;
						}
						if (self.temp > 440.f)
						{
							burnTimer = 1024 + (storedEnergy > 24 ? 96 : storedEnergy * 4);
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_H2);
							hydrogenationFactor = 10;
						}
						else
						{
							self.temp = restrict_flt(self.temp + 20.365f + storedEnergy * storedEnergy * 1.5f, MIN_TEMP, MAX_TEMP);
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_H2);
							hydrogenationFactor += 1;
						}
						break;
						
					case PT_CO2:
						if (hydrogenationFactor + carbonationFactor >= 10)
						{
							continue;
						}
						sim->part_kill(ID(neighborData));
						carbonationFactor += 1;
						break;
						
					case PT_SPRK:
						if (parts_avg(i, ID(neighborData), PT_INSL) == PT_INSL)
						{
							break;
						}
						if (hydrogenationFactor + carbonationFactor >= 5)
						{
							continue; // too impure to do battery things.
						}
						if (neighbor.ctype == PT_PSCN && neighbor.life == 3 && !charged && !burnTimer)
						{
							charged = true;
						}
						break;
						
					case PT_NSCN:
						if (parts_avg(i, ID(neighborData), PT_INSL) == PT_INSL)
						{
							break;
						}
						if (neighbor.life == 0 && storedEnergy > 0 && !burnTimer)
						{
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_SPRK);
							neighbor.life = 4;
							neighbor.ctype = PT_NSCN;
							discharged = true;
						}
						break;
						
					case PT_FIRE:
						if (self.temp > 440.f && RNG::Ref().chance(1, 40) && hydrogenationFactor < 6)
						{
							burnTimer = 1013;
							hydrogenationFactor += 1;
						}
						break;
						
					case PT_O2:
						if (burnTimer > 1000 && RNG::Ref().chance(1, 10))
						{
							sim->part_change_type(i, x, y, PT_PLSM);
							sim->part_change_type(ID(neighborData), x + rx, y + ry, PT_PLSM);
							sim->air->pv[y / CELL][x / CELL] += 4.0;
							return 0;
						}						
						break;
				}
			}
		}
	}
	if (charged)
	{
		storedEnergy += 1;
		burnTimer = 8;
	}
	if (discharged)
	{
		storedEnergy -= 1;
		burnTimer = 8;
	}
	
	for (int trade = 0; trade < 9; ++trade)
	{
		int rx = RNG::Ref().between(-3, 3);
		int ry = RNG::Ref().between(-3, 3);
		if (BOUNDS_CHECK && (rx || ry))
		{
			int neighborData = pmap[y + ry][x + rx];
			if (TYP(neighborData) != PT_LITH)
			{
				continue;
			}
			particle &neighbor = parts[ID(neighborData)];
			
			int &neighborStoredEnergy = neighbor.ctype;
			// Transfer overcharge explosion status to nearby LITH
			if (burnTimer < 1000 && storedEnergy > 90 && neighbor.life > 1000)
				burnTimer = 1024;
			if (storedEnergy > neighborStoredEnergy)
			{
				int transfer = storedEnergy - neighborStoredEnergy;
				transfer -= transfer / 2;
				neighborStoredEnergy += transfer;
				storedEnergy -= transfer;
				break;
			}
		}
	}

	// Overcharged - begin explosion
	if (burnTimer < 1000 && storedEnergy >= 100)
		burnTimer = 1024;
	if (burnTimer == 1000)
	{
		sim->part_change_type(i, x, y, PT_LAVA);
		if (carbonationFactor < 3)
		{
			self.temp = 500.f + storedEnergy * 10;
			self.ctype = PT_LITH;
		}
		else
		{
			self.temp = 2000.f + storedEnergy * 10;
			self.ctype = PT_GLAS;
		}
	}
	return 0;
}

int LITH_graphics(GRAPHICS_FUNC_ARGS)
{
	// Exploding lith
	if (cpart->life >= 1000)
	{
		int colour = 0xFFA040;
		*colr = PIXR(colour);
		*colg = PIXG(colour);
		*colb = PIXB(colour);
		*pixel_mode |= PMODE_FLARE | PMODE_GLOW;
	}
	// Charged lith
	else if (cpart->ctype > 0)
	{
		int mult = RNG::Ref().between(cpart->ctype / 3, cpart->ctype) / 15;
		mult = std::min(6, mult);
		*colr -= 30 * mult;
		*colb += 20 * mult;
		*pixel_mode |= PMODE_FLARE | PMODE_GLOW;
	}
	return 0;
}

void LITH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LITH";
	elem->Name = "LITH";
	elem->Colour = PIXPACK(0xB6AABF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.2f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.2f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 15;

	elem->Weight = 17;

	elem->HeatConduct = 70;
	elem->Description = "Lithium. Reactive element that explodes on contact with water.";

	elem->Properties = TYPE_PART | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 453.65f;
	elem->HighTemperatureTransitionElement = PT_LAVA;
	
	elem->Update = &LITH_update;
	elem->Graphics = &LITH_graphics;
	elem->Init = &LITH_init_element;
}
