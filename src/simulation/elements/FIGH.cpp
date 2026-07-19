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
#include "simulation/elements/FIGH.h"
#include "simulation/ToolNumbers.h"

int STKM_graphics(GRAPHICS_FUNC_ARGS);

void FIGH_ElementDataContainer::NewFighter(Simulation *sim, int fighterID, int i, int elem)
{
	static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).InitLegs(&fighters[fighterID], i);
	if (elem >= 0 && elem < PT_NUM)
		fighters[fighterID].elem = elem;
	fighters[fighterID].spwn = 1;
}

int FIGH_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp < 0 || parts[i].tmp >= static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).MaxFighters())
	{
		sim->part_kill(i);
		return 1;
	}

	Stickman * figh = static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Get((unsigned char)parts[i].tmp);
	Stickman * player = static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1();
	Stickman * player2 = static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2();

	parts[i].tmp2 = 0; //0 - stay in place, 1 - seek a stickman

	//Set target coords
	int tarx = 0, tary = 0;
	if (sim->elementCount[PT_STKM2] > 0)
	{
		if (sim->elementCount[PT_STKM] > 0 && (pow(player->legs[2]-x, 2) + pow(player->legs[3]-y, 2)) <= (pow(player2->legs[2]-x, 2) + pow(player2->legs[3]-y, 2)))
		{
			tarx = (int)player->legs[2];
			tary = (int)player->legs[3];
		}
		else
		{
			tarx = (int)player2->legs[2];
			tary = (int)player2->legs[3];
		}
		parts[i].tmp2 = 1;
	}
	else if (sim->elementCount[PT_STKM] > 0)
	{
		tarx = (int)player->legs[2];
		tary = (int)player->legs[3];
		parts[i].tmp2 = 1;
	}

	switch (parts[i].tmp2)
	{
	case 1:
		if ((pow(tarx-x, 2.0f) + pow(tary-y, 2.0f))<600)
		{
			if (figh->elem == PT_LIGH || figh->elem == PT_NEUT 
			        || sim->elements[figh->elem].Properties & (PROP_DEADLY | PROP_RADIOACTIVE)
			        || sim->elements[figh->elem].DefaultProperties.temp >= 323
			        || sim->elements[figh->elem].DefaultProperties.temp <= 243
			        || figh->fan)
				figh->comm = (int)figh->comm | 0x08;
		}
		else if (tarx<x)
		{
			if(figh->rocketBoots || !(sim->EvalMove(PT_FIGH, (int)figh->legs[4]-10, (int)figh->legs[5]+6)
				&& sim->EvalMove(PT_FIGH, (int)figh->legs[4]-10, (int)figh->legs[5]+3)))
				figh->comm = 0x01;
			else
				figh->comm = 0x02;

			if (figh->rocketBoots)
			{
				if (tary<y)
					figh->comm = (int)figh->comm | 0x04;
			}
			else if (!sim->EvalMove(PT_FIGH, (int)figh->legs[4]-4, (int)figh->legs[5]-1)
					|| !sim->EvalMove(PT_FIGH, (int)figh->legs[12]-4, (int)figh->legs[13]-1)
					|| sim->EvalMove(PT_FIGH, (int)(2*figh->legs[4]-figh->legs[6]), (int)figh->legs[5]+5))
				figh->comm = (int)figh->comm | 0x04;
		}
		else
		{
			if (figh->rocketBoots || !(sim->EvalMove(PT_FIGH, (int)figh->legs[12]+10, (int)figh->legs[13]+6)
				&& sim->EvalMove(PT_FIGH, (int)figh->legs[12]+10, (int)figh->legs[13]+3)))
				figh->comm = 0x02;
			else
				figh->comm = 0x01;

			if (figh->rocketBoots)
			{
				if (tary<y)
					figh->comm = (int)figh->comm | 0x04;
			}
			else if (!sim->EvalMove(PT_FIGH, (int)figh->legs[4]+4, (int)figh->legs[5]-1)
					|| !sim->EvalMove(PT_FIGH, (int)figh->legs[4]+4, (int)figh->legs[5]-1)
					|| sim->EvalMove(PT_FIGH, (int)(2*figh->legs[12]-figh->legs[14]), (int)figh->legs[13]+5))
				figh->comm = (int)figh->comm | 0x04;
		}
		break;
	default:
		figh->comm = 0;
		break;
	}

	figh->pcomm = figh->comm;

	static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).Run(figh, UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

bool FIGH_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).CanAlloc();
}

void FIGH_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to == PT_FIGH)
	{
		sim->parts[i].tmp = static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Alloc();
		if (sim->parts[i].tmp >= 0)
			static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).NewFighter(sim, sim->parts[i].tmp, i, PT_DUST);
	}
	else
	{
		static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Free((unsigned char)sim->parts[i].tmp);
	}
}

void FIGH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FIGH";
	elem->Name = "FIGH";
	elem->Colour = COLPACK(0xFFE0A0);
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
	elem->Description = "Fighter. Tries to kill stickmen. You must first give it an element to kill him with.";

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

	elem->Update = &FIGH_update;
	elem->Graphics = &STKM_graphics;
	elem->Func_Create_Allowed = &FIGH_create_allowed;
	elem->Func_ChangeType = &FIGH_ChangeType;
	elem->Init = &FIGH_init_element;

	sim->elementData[t].reset(new FIGH_ElementDataContainer);
}
