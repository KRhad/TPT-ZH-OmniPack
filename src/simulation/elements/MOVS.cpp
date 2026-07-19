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
#include "simulation/ElementsCommon.h"
#include "MOVS.h"

void rotate(float *x, float *y, float angle)
{
	float cos = cosf(angle), sin = sinf(angle);
	float newx = cos**x-sin**y, newy = sin**x+cos**y;
	*x = newx, *y = newy;
}

int MOVS_update(UPDATE_FUNC_ARGS)
{
	int bn = parts[i].tmp2, type, bounce = 2;
	float tmp3 = parts[i].tmp3, tmp4 = parts[i].tmp4;

	MovingSolid *movingSolid = static_cast<MOVS_ElementDataContainer&>(*sim->elementData[PT_MOVS]).GetMovingSolid(bn);
	if (!movingSolid || (parts[i].flags&FLAG_DISAPPEAR))
		return 0;
	//center control particle was killed, ball slowly falls apart
	if (!movingSolid->index)
	{
		if (RNG::Ref().chance(1, 500))
		{
			sim->part_kill(i);
			return 1;
		}
	}
	//determine rotated x and y coordinates relative to center (if rotation is on)
	else
	{
		tmp3 = parts[i].tmp3;
		tmp4 = parts[i].tmp4;
		if (sim->msRotation)
			rotate(&tmp3, &tmp4, movingSolid->rotationOld);
	}
	//kill moving solid control particle with a lot of pressure (other ones disappear at 30 pressure)
	if (!tmp3 && !tmp4 && (sim->air->pv[y/CELL][x/CELL] > 10 || sim->air->pv[y/CELL][x/CELL] < -10))
	{
		sim->part_kill(i);
		return 1;
	}
	type = TYP(pmap[y+1][x]);
	//bottom side collision
	if (tmp4 > 0 && type && y+1 < YRES && ((type != PT_MOVS && !sim->EvalMove(PT_MOVS, x, y+1)) || (type == PT_MOVS && parts[ID(pmap[y+1][x])].tmp2 != bn) || sim->IsWallBlocking(x, y+1, PT_MOVS)))
	{
		parts[i].vy -= tmp4*bounce;
		movingSolid->rotation -= tmp3/50000;
	}
	type = TYP(pmap[y-1][x]);
	//top side collision
	if (tmp4 < 0 && type && y-1 >= 0 && ((type != PT_MOVS && !sim->EvalMove(PT_MOVS, x, y-1)) || (type == PT_MOVS && parts[ID(pmap[y-1][x])].tmp2 != bn) || sim->IsWallBlocking(x, y-1, PT_MOVS)))
	{
		parts[i].vy -= tmp4*bounce;
		movingSolid->rotation -= tmp3/50000;
	}
	type = TYP(pmap[y][x+1]);
	//right side collision
	if (tmp3 > 0 && type && x+1 < XRES && ((type != PT_MOVS && !sim->EvalMove(PT_MOVS, x+1, y)) || (type == PT_MOVS && parts[ID(pmap[y][x+1])].tmp2 != bn) || sim->IsWallBlocking(x+1, y, PT_MOVS)))
	{
		parts[i].vx -= tmp3*bounce;
		movingSolid->rotation -= tmp3/50000;
	}
	type = TYP(pmap[y][x-1]);
	//left side collision
	if (tmp3 < 0 && type && x-1 >= 0 && ((type != PT_MOVS && !sim->EvalMove(PT_MOVS, x-1, y)) || (type == PT_MOVS && parts[ID(pmap[y][x-1])].tmp2 != bn) || sim->IsWallBlocking(x-1, y, PT_MOVS)))
	{
		parts[i].vx -= tmp3*bounce;
		movingSolid->rotation -= tmp3/50000;
	}
	return 0;
}

bool MOVS_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	if (static_cast<MOVS_ElementDataContainer&>(*sim->elementData[PT_MOVS]).GetNumBalls() >= 255 || pmap[y][x])
		return false;
	return true;
}

void MOVS_create(ELEMENT_CREATE_FUNC_ARGS)
{
	if (v == 2 || static_cast<MOVS_ElementDataContainer&>(*sim->elementData[PT_MOVS]).IsCreatingSolid())
	{
		static_cast<MOVS_ElementDataContainer&>(*sim->elementData[PT_MOVS]).CreateMovingSolid(i, x, y);
	}
	else if (v == 1)
	{
		static_cast<MOVS_ElementDataContainer&>(*sim->elementData[PT_MOVS]).CreateMovingSolidCenter(i);
	}
	else
	{
		parts[i].tmp2 = 255;
		parts[i].tmp3 = RNG::Ref().between(-10, 10);
		parts[i].tmp4 = RNG::Ref().between(-10, 10);
	}
}

void MOVS_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to != PT_MOVS)
	{
		MovingSolid *movingSolid = static_cast<MOVS_ElementDataContainer&>(*sim->elementData[PT_MOVS]).GetMovingSolid(parts[i].tmp2);
		if (movingSolid && !(parts[i].flags&FLAG_DISAPPEAR))
		{
			movingSolid->particleCount--;
			if (movingSolid->index-1 == i)
				movingSolid->index = 0;
		}
	}
}

void MOVS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_MOVS";
	elem->Name = "BALL";
	elem->Colour = COLPACK(0x0010A0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.004f * CFDS;
	elem->AirLoss = 0.92f;
	elem->Loss = 0.80f;
	elem->Collision = 0.00f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 85;

	elem->DefaultProperties.temp = R_TEMP + 273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Moving solid. Acts like a bouncy ball.";

	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = -25.0f;
	elem->LowPressureTransitionElement = PT_NONE;
	elem->HighPressureTransitionThreshold = 25.0f;
	elem->HighPressureTransitionElement = PT_NONE;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &MOVS_update;
	elem->Graphics = NULL;
	elem->Func_Create_Allowed = &MOVS_create_allowed;
	elem->Func_Create = &MOVS_create;
	elem->Func_ChangeType = &MOVS_ChangeType;
	elem->Init = &MOVS_init_element;

	sim->elementData[t].reset(new MOVS_ElementDataContainer);
}
#endif
