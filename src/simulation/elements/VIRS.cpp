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

int VIRS_update(UPDATE_FUNC_ARGS)
{
	//tmp3 measures how many frames until it is cured (0 if still actively spreading and not being cured)
	//tmp4 measures how many frames until it dies
	int rndstore = RNG::Ref().gen();
	if (parts[i].tmp3)
	{
		parts[i].tmp3 -= (rndstore&0x1) ? 0:1;
		//has been cured, so change back into the original element
		if (parts[i].tmp3 <= 0)
		{
			part_change_type(i,x,y,parts[i].tmp2);
			parts[i].tmp2 = 0;
			parts[i].tmp3 = 0;
			parts[i].tmp4 = 0;
			return 1;
		}

		//cured virus isn't allowed in below code
		return 0;
	}
	//decrease tmp4 so it slowly dies
	if (parts[i].tmp4 > 0)
	{
		if (!(rndstore & 0x7) && --parts[i].tmp4 <= 0)
		{
			sim->part_kill(i);
			return 1;
		}
		rndstore >>= 3;
	}

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
		{
			if (BOUNDS_CHECK && (rx || ry))
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				//spread "being cured" state
				if (parts[ID(r)].tmp3 && (TYP(r) == PT_VIRS || TYP(r) == PT_VRSS || TYP(r) == PT_VRSG))
				{
					parts[i].tmp3 = parts[ID(r)].tmp3 + ((rndstore & 0x3) ? 2:1);
					return 0;
				}
				//soap cures virus
				else if (TYP(r) == PT_SOAP)
				{
					parts[i].tmp3 += 10;
					if (!(rndstore & 0x3))
						sim->part_kill(ID(r));
					return 0;
				}
				else if (TYP(r) == PT_PLSM)
				{
					if (surround_space && 10 + RNG::Ref().chance(sim->air->pv[(y+ry)/CELL][(x+rx)/CELL], 100))
					{
						sim->part_create(i, x, y, PT_PLSM);
						return 1;
					}
				}
				else if (TYP(r) != PT_VIRS && TYP(r) != PT_VRSS && TYP(r) != PT_VRSG && !(sim->elements[TYP(r)].Properties&PROP_INDESTRUCTIBLE))
				{
					if (!(rndstore & 0x7))
					{
						parts[ID(r)].tmp2 = TYP(r);
						parts[ID(r)].tmp3 = 0;
						if (parts[i].tmp4)
							parts[ID(r)].tmp4 = parts[i].tmp4 + 1;
						else
							parts[ID(r)].tmp4 = 0;
						if (parts[ID(r)].temp < 305.0f)
							sim->part_change_type(ID(r), x + rx, y + ry, PT_VRSS);
						else if (parts[ID(r)].temp > 673.0f)
							sim->part_change_type(ID(r), x + rx, y + ry, PT_VRSG);
						else
							sim->part_change_type(ID(r), x + rx, y + ry, PT_VIRS);
					}
					rndstore >>= 3;
				}
				// Protons make VIRS last forever
				else if (TYP(photons[y+ry][x+rx]) == PT_PROT)
				{
					parts[i].tmp4 = 0;
				}
			}
			// Reset rndstore only once, halfway through
			else if (!rx && !ry)
				rndstore = RNG::Ref().gen();
		}
	return 0;
}

int VIRS_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_BLUR;
	*pixel_mode |= NO_DECO;
	return 1;
} 

void VIRS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VIRS";
	elem->Name = "VIRS";
	elem->Colour = COLPACK(0xFE11F6);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
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
	elem->Hardness = 20;

	elem->Weight = 31;

	elem->DefaultProperties.temp = 72.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Virus. Turns everything it touches into virus.";

	elem->Properties = TYPE_LIQUID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 305.0f;
	elem->LowTemperatureTransitionElement = PT_VRSS;
	elem->HighTemperatureTransitionThreshold = 673.0f;
	elem->HighTemperatureTransitionElement = PT_VRSG;

	elem->DefaultProperties.tmp4 = 250;

	elem->Update = &VIRS_update;
	elem->Graphics = &VIRS_graphics;
	elem->Init = &VIRS_init_element;
}
