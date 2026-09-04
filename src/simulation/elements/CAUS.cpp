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

int CAUS_update(UPDATE_FUNC_ARGS)
{
	bool converted = false;
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_GAS)
				{
					if (sim->air->pv[(y+ry)/CELL][(x+rx)/CELL] > 3)
					{
						sim->part_change_type(ID(r), x+rx, y+ry, PT_RFRG);
						sim->part_change_type(i, x, y, PT_RFRG);
						converted = true;
					}
				}
				else if (TYP(r)!=PT_ACID && TYP(r)!=PT_CAUS && TYP(r)!=PT_RFRG && TYP(r)!=PT_RFGL)
				{
					if ((!(sim->elements[TYP(r)].Properties & PROP_CLONE) &&
						!(sim->elements[TYP(r)].Properties & PROP_INDESTRUCTIBLE) &&
						((TYP(r) != PT_FOG && TYP(r) != PT_RIME) || parts[ID(r)].tmp <= 5) &&
						RNG::Ref().chance(sim->elements[TYP(r)].Hardness, 1000)) && parts[i].life>=50)
					{
						if (parts_avg(i, ID(r),PT_GLAS)!= PT_GLAS)//GLAS protects stuff from acid
						{
							float newtemp = ((60.0f-(float)sim->elements[TYP(r)].Hardness))*7.0f;
							if(newtemp < 0){
								newtemp = 0;
							}
							parts[i].temp += newtemp;
							parts[i].life--;
							sim->part_kill(ID(r));
							continue;
						}
					}
					else if (parts[i].life<=50)
					{
						sim->part_kill(i);
						return 1;
					}
				}
			}
	return converted;
}

void CAUS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CAUS";
	elem->Name = "CAUS";
	elem->Colour = COLPACK(0x80FFA0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 1.50f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "腐蚀性气体。作用类似酸。";

	elem->Properties = TYPE_GAS|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 75;

	elem->Update = &CAUS_update;
	elem->Graphics = NULL;
	elem->Init = &CAUS_init_element;
}
