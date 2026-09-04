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

int GLAS_update(UPDATE_FUNC_ARGS)
{
	auto press = int(sim->air->pv[y/CELL][x/CELL] * 64);
	auto diff = press - parts[i].tmp3;

	// Determine whether the GLAS is chemically strengthened via .life setting. (250 = Max., 16 = Min.)
	int strength = (parts[i].life / 120) + 16;
	if (strength < 16)
		strength = 16;
	if (diff > strength || diff < -1 * strength)
	{
		sim->part_change_type(i, x, y, PT_BGLA);
	}
	parts[i].tmp3 = press;

	// Liquid nitrogen condenses on chemically strengthened glass
	if ((strength > 200) && (parts[i].temp < 77.0f) && (sim->air->hv[y/CELL][x/CELL] < 77.0f)
		&& (sim->air->pv[y/CELL][x/CELL] > 5.0f) && RNG::Ref().chance(1, 100))
	{
		// Sample 4 adjacent cells
		auto adj = RNG::Ref().between(0, 3);
		auto rx = (1 - 2 * (adj % 2)) * (1 - adj / 2);
		auto ry = (1 - 2 * (adj % 2)) * (adj / 2);

		auto r = pmap[y + ry][x + rx];
		// If found an empty spot around glass
		if (!r)
		{
			auto np = sim->part_create(-1, x + rx, y + ry, PT_LNTG);
			if (np>-1)
			{
				sim->air->pv[y/CELL][x/CELL] -= 1.0f;
				parts[i].temp += 200.0f;
			}
		}
	}

	return 0;
}

void GLAS_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp3 = int(sim->air->pv[y/CELL][x/CELL] * 64);
}

void GLAS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GLAS";
	elem->Name = "GLAS";
	elem->Colour = COLPACK(0x404040);
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
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "玻璃。可熔化。受压碎裂并折射光子。";

	elem->Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_PHOTPASS | PROP_HOT_GLOW | PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1973.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &GLAS_update;
	elem->Graphics = NULL;
	elem->Func_Create = &GLAS_create;
	elem->Init = &GLAS_init_element;
}
