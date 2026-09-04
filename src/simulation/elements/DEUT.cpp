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
#include "common/tpt-minmax.h"

int DEUT_update(UPDATE_FUNC_ARGS)
{
	float gravtot = fabs(sim->grav->gravy[(y/CELL)*(XRES/CELL)+(x/CELL)]) + fabs(sim->grav->gravx[(y/CELL)*(XRES/CELL)+(x/CELL)]);
	// Prevent division by 0
	float temp = std::max(1.0f, (parts[i].temp + 1));
	int maxlife = static_cast<int>((10000 / (temp + 1)) - 1);
	// no idea what this line was intended to do, but kept for compatibility
	if (RNG::Ref().chance(10000 % static_cast<int>(temp + 1), static_cast<int>(temp + 1)))
		maxlife++;
	// Compress when Newtonian gravity is applied
	// multiplier=1 when gravtot=0, multiplier -> 5 as gravtot -> inf
	maxlife = (int)(maxlife*(5.0f - 8.0f/(gravtot+2.0f)));
	if (parts[i].life < maxlife)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r || (parts[i].life >=maxlife))
						continue;
					if (TYP(r)==PT_DEUT && RNG::Ref().chance(1, 3))
					{
						// If neighbour life+1 fits in the free capacity for this particle, absorb neighbour
						// Condition is written in this way so that large neighbour life values don't cause integer overflow
						if (parts[ID(r)].life <= maxlife - parts[i].life - 1)
						{
							parts[i].life += parts[ID(r)].life + 1;
							sim->part_kill(ID(r));
						}
					}
				}
	}
	else
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					//Leave if there is nothing to do
					if (parts[i].life <= maxlife)
						goto trade;

					int r = pmap[y+ry][x+rx];
					if ((!r)&&parts[i].life>=1)//if nothing then create deut
					{
						int np = sim->part_create(-1,x+rx,y+ry,PT_DEUT);
						if (np<0) continue;
						parts[i].life--;
						parts[np].temp = parts[i].temp;
						parts[np].life = 0;
					}
				}

trade:
	for (int trade = 0; trade<4; trade ++)
	{
		int rx = RNG::Ref().between(-2, 2);
		int ry = RNG::Ref().between(-2, 2);
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r)==PT_DEUT&&(parts[i].life>parts[ID(r)].life)&&parts[i].life>0)//diffusion
			{
				int temp = parts[i].life - parts[ID(r)].life;
				if (temp ==1)
				{
					parts[ID(r)].life ++;
					parts[i].life --;
				}
				else if (temp>0)
				{
					parts[ID(r)].life += temp/2;
					parts[i].life -= temp/2;
				}
			}
		}
	}
	return 0;
}

int DEUT_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life>=240)
	{
		*firea = 60;
		*firer = *colr += 255;
		*fireg = *colg += 255;
		*fireb = *colb += 255;
		*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	}
	else if(cpart->life>0)
	{
		*colr += cpart->life*1;
		*colg += cpart->life*2;
		*colb += cpart->life*3;
		*pixel_mode |= PMODE_BLUR;
	}
	else
	{
		*pixel_mode |= PMODE_BLUR;
	}
	return 0;
}

void DEUT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DEUT";
	elem->Name = "DEUT";
	elem->Colour = COLPACK(0x00153F);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 19;

	elem->Weight = 31;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "重水。遇冷浓缩，与中子或质子接触爆炸。";

	elem->Properties = TYPE_LIQUID|PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 10;

	elem->Update = &DEUT_update;
	elem->Graphics = &DEUT_graphics;
	elem->Init = &DEUT_init_element;
}
