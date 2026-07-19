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
#include "simulation/elements/STKM.h"

bool SPAWN2_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2()->spawnID == -1;
}

void SPAWN2_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to == PT_SPAWN2)
	{
		Stickman *player = static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2();
		if (player->spawnID == -1)
			player->spawnID = i;
	}
	else
	{
		Stickman *player = static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2();
		if (player->spawnID == i)
			player->spawnID = -1;
	}
}

void SPAWN2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SPAWN2";
	elem->Name = "SPWN2";
	elem->Colour = COLPACK(0xAAAAAA);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "STK2 spawn point.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Func_Create_Allowed = &SPAWN2_create_allowed;
	elem->Func_ChangeType = &SPAWN2_ChangeType;
	elem->Init = &SPAWN2_init_element;
}
