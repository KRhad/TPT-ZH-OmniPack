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

#ifndef NOMOD
#include <cstring>
#include "simulation/ElementsCommon.h"
#include "ANIM.h"

int ANIM_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life == 10)
	{
		parts[i].tmp--;
		if (parts[i].tmp <= 0)
		{
			parts[i].tmp = (int)(parts[i].temp - 273.15f);
			parts[i].tmp2++;
		}
	}
	if (parts[i].tmp2 > parts[i].ctype)
	{
		parts[i].tmp2 = 0;
	}
	if (!(static_cast<ANIM_ElementDataContainer&>(*sim->elementData[PT_ANIM]).isValid(i, parts[i].tmp2)))
	{
		sim->part_kill(i);
		return 1;
	}
	// Put current frame color in dcolor, used by graphics function even when deco is turned off
	parts[i].dcolour = static_cast<ANIM_ElementDataContainer&>(*sim->elementData[PT_ANIM]).GetColor(i, parts[i].tmp2);
	return 0;
}

int ANIM_graphics(GRAPHICS_FUNC_ARGS)
{
	//if decorations are even set (black deco has alpha set)
	if (cpart->dcolour)
	{
		*cola = COLA(cpart->dcolour);
		*colr = COLR(cpart->dcolour);
		*colg = COLG(cpart->dcolour);
		*colb = COLB(cpart->dcolour);
	}
	else
		return 0;

	if (cpart->life < 10)
	{
		*colr = (int)(*colr / (5.5f - cpart->life / 2.0f));
		*colg = (int)(*colg / (5.5f - cpart->life / 2.0f));
		*colb = (int)(*colb / (5.5f - cpart->life / 2.0f));
	}

	*pixel_mode |= NO_DECO;
	return 0;
}

void ANIM_create(ELEMENT_CREATE_FUNC_ARGS)
{
	static_cast<ANIM_ElementDataContainer&>(*sim->elementData[PT_ANIM]).InitlializePart(i);
}

void ANIM_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to != PT_ANIM)
		static_cast<ANIM_ElementDataContainer&>(*sim->elementData[PT_ANIM]).FreePart(i);
}

void ANIM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ANIM";
	elem->Name = "ANIM";
	elem->Colour = COLPACK(0x505050);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP + 273.15f;
	elem->DefaultProperties.life = 10;
	elem->DefaultProperties.ctype = 0;
	elem->DefaultProperties.tmp = 1;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Animated Liquid Crystal. Can show multiple frames, use left/right in the deco editor.";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &ANIM_update;
	elem->Graphics = &ANIM_graphics;
	elem->Func_Create = &ANIM_create;
	elem->Func_ChangeType = &ANIM_ChangeType;
	elem->Init = &ANIM_init_element;

	sim->elementData[t].reset(new ANIM_ElementDataContainer());
}
#endif
