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

int STKM_graphics(GRAPHICS_FUNC_ARGS);

int STKM2_update(UPDATE_FUNC_ARGS)
{
	static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).Run(static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2(), UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

bool STKM2_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return sim->elementCount[PT_STKM2]<=0 && !static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2()->spwn;
}

void STKM2_create(ELEMENT_CREATE_FUNC_ARGS)
{
	int id = sim->part_create(-3, x, y, PT_SPAWN2);
	if (id >= 0)
		static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2()->spawnID = id;
}

void STKM2_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to == PT_STKM2)
		static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).NewStickman2(i, -1);
	else
		static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2()->spwn = 0;
}

void STKM2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STKM2";
	elem->Name = "STK2";
	elem->Colour = COLPACK(0x6464FF);
#ifndef TOUCHUI
	elem->MenuVisible = 1;
#else
	elem->MenuVisible = 0;
#endif
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.2f;
	elem->Loss = 1.0f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->NewtonianGravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->HotAir = 0.00f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 50;

	elem->DefaultProperties.temp = R_TEMP + 14.6f + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "second stickman. Don't kill him! Control with wasd.";

	elem->Properties = PROP_NOCTYPEDRAW;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 620.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->DefaultProperties.life = 100;

	elem->Update = &STKM2_update;
	elem->Graphics = &STKM_graphics;
	elem->Func_Create_Allowed = &STKM2_create_allowed;
	elem->Func_Create = &STKM2_create;
	elem->Func_ChangeType = &STKM2_ChangeType;
	elem->Init = &STKM2_init_element;
}
