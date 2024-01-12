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

int GOLD_update(UPDATE_FUNC_ARGS)
{
	static int checkCoordsX[] = { -4, 4, 0, 0 };
	static int checkCoordsY[] = { 0, 0, -4, 4 };
	//Find nearby rusted iron (BMTL with tmp 1+)
	for (int j = 0; j < 8; j++)
	{
		int rndstore = RNG::Ref().gen();
		int rx = (rndstore % 9)-4;
		rndstore >>= 4;
		int ry = (rndstore % 9)-4;
		if (!rx != !ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r) == PT_BMTL && parts[ID(r)].tmp)
			{
				parts[ID(r)].tmp = 0;
				sim->part_change_type(ID(r), x+rx, y+ry, PT_IRON);
			}
		}
	}
	//Find sparks
	if (!parts[i].life)
	{
		for (int j = 0; j < 4; j++)
		{
			int rx = checkCoordsX[j];
			int ry = checkCoordsY[j];
			if (!rx != !ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_SPRK && parts[ID(r)].life && parts[ID(r)].life < 4)
				{
					sim->spark_conductive(i, x, y);
					return 1;
				}
			}
		}
	}
	if (TYP(photons[y][x]) == PT_NEUT)
	{
		if (RNG::Ref().chance(1, 7))
		{
			sim->part_kill(ID(photons[y][x]));
		}
	}
	return 0;
}

int GOLD_graphics(GRAPHICS_FUNC_ARGS)
{
	int rndstore = RNG::Ref().gen();
	*colr += (rndstore % 10) - 5;
	rndstore >>= 4;
	*colg += (rndstore % 10)- 5;
	rndstore >>= 4;
	*colb += (rndstore % 10) - 5;
	return 0;
}

void GOLD_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GOLD";
	elem->Name = "GOLD";
	elem->Colour = COLPACK(0xDCAD2C);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 0;
	elem->PhotonReflectWavelengths = 0x3C038100;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Corrosion resistant metal, will reverse corrosion of iron.";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_HOT_GLOW|PROP_LIFE_DEC|PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1337.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &GOLD_update;
	elem->Graphics = &GOLD_graphics;
	elem->Init = &GOLD_init_element;
}
