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

void DeutImplosion(Simulation *sim, int n, int x, int y, float temp, int t)
{
	n = (n/50);
	if (n < 1)
		n = 1;
	else if (n > 340)
		n = 340;

	for (int c = 0; c < n; c++)
	{
		int i = sim->part_create(-3, x, y, t);
		if (i >= 0)
			parts[i].temp = temp;
		else if (sim->pfree < 0)
			break;
	}
	sim->air->pv[y/CELL][x/CELL] -= (6.0f * CFDS) * n;
	return;
}

int PROT_update(UPDATE_FUNC_ARGS)
{
	sim->air->pv[y/CELL][x/CELL] -= .003f;
	int under = pmap[y][x];
	int utype = TYP(under);
	int uID = ID(under);
	switch (utype)
	{
	case PT_SPRK:
	{
		// Remove active sparks
		int sparked = parts[uID].ctype;
		if (sparked > 0 && sparked < PT_NUM && sim->elements[sparked].Enabled)
		{
			sim->part_change_type(uID, x, y, sparked);
			parts[uID].life = 44 + parts[uID].life;
			parts[uID].ctype = 0;
		}
		break;
	}
	case PT_DEUT:
		if (RNG::Ref().chance(-(sim->air->pv[y/CELL][x/CELL] - 4) + (parts[uID].life / 100), 200))
		{
			DeutImplosion(sim, parts[uID].life, x, y, restrict_flt(parts[uID].temp + parts[uID].life*500, MIN_TEMP, MAX_TEMP), PT_PROT);
			sim->part_kill(uID);
		}
		break;
	case PT_LCRY:
		// Powered LCRY reaction: PROT->PHOT
		if (parts[uID].life > 5 && RNG::Ref().chance(1, 10))
		{
			sim->part_change_type(i, x, y, PT_PHOT);
			parts[i].life *= 2;
			parts[i].ctype = 0x3FFFFFFF;
		}
		break;
	case PT_EXOT:
		parts[uID].ctype = PT_PROT;
		break;
	case PT_NONE:
		if (parts[i].life && !--parts[i].life)
		{
			sim->part_kill(i);
			return 1;
		}
		break;
	case PT_WIFI:
	{
		float change;
		if (parts[i].temp < 173.15f)
			change = -1000.0f;
		else if (parts[i].temp < 273.15f)
			change = -100.0f;
		else if (parts[i].temp > 473.15f)
			change = 1000.0f;
		else if (parts[i].temp > 373.15f)
			change = 100.0f;
		else
			change = 0.0f;
		parts[uID].temp = restrict_flt(parts[uID].temp + change, MIN_TEMP, MAX_TEMP);
		break;
	}
	default:
		// Set off explosives (only when hot because it wasn't as fun when it made an entire save explode)
		if (parts[i].temp > 273.15f + 500.0f && (sim->elements[utype].Flammable || sim->elements[utype].Explosive || utype == PT_BANG))
		{
			sim->part_create(uID, x, y, PT_FIRE);
			parts[uID].temp += restrict_flt(sim->elements[utype].Flammable*5.0f, MIN_TEMP, MAX_TEMP);
			sim->air->pv[y/CELL][x/CELL] += 1.00f;
		}
		// Prevent inactive sparkable elements from being sparked
		else if ((sim->elements[utype].Properties & PROP_CONDUCTS) && parts[uID].life <= 4)
		{
			parts[uID].life = 40 + parts[uID].life;
		}
		break;
	}
	// Make temp of other things closer to it's own temperature. This will change temp of things that don't conduct, and won't change the PROT's temperature
	if (utype && utype != PT_WIFI)
		parts[uID].temp = restrict_flt(parts[uID].temp -(parts[uID].temp - parts[i].temp) / 4.0f, MIN_TEMP, MAX_TEMP);
	
	// If this proton has collided with another last frame, change it into a heavier element
	if (parts[i].tmp)
	{
		int newID, element;
		if (parts[i].tmp > 500000)
			element = PT_SING; // Particle accelerators are known to create earth-destroying black holes
		else if (parts[i].tmp > 700)
			element = PT_PLUT;
		else if (parts[i].tmp > 420)
			element = PT_URAN;
		else if (parts[i].tmp > 310)
			element = PT_POLO;
		else if (parts[i].tmp > 250)
			element = PT_PLSM;
		else if (parts[i].tmp > 100)
			element = PT_O2;
		else if (parts[i].tmp > 50)
			element = PT_CO2;
		else
			element = PT_NBLE;
		newID = sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), element);
		if (newID >= 0)
			parts[newID].temp = restrict_flt(100.0f*parts[i].tmp, MIN_TEMP, MAX_TEMP);
		sim->part_kill(i);
		return 1;
	}
	//collide with other protons to make heavier materials
	int ahead = photons[y][x];
	if (ID(ahead) != i && TYP(ahead) == PT_PROT)
	{
		float velocity1 = powf(parts[i].vx, 2.0f) + powf(parts[i].vy, 2.0f);
		float velocity2 = powf(parts[ID(ahead)].vx, 2.0f) + powf(parts[ID(ahead)].vy, 2.0f);
		float direction1 = atan2f(-parts[i].vy, parts[i].vx);
		float direction2 = atan2f(-parts[ID(ahead)].vy, parts[ID(ahead)].vx);
		float difference = direction1 - direction2;
		if (difference < 0)
			difference += 6.28319f;

		if (difference > 3.12659f && difference < 3.15659f && velocity1 + velocity2 > 10.0f)
		{
			parts[ID(ahead)].tmp += (int)(velocity1 + velocity2);
			sim->part_kill(i);
			return 1;
		}
	}
	return 0;
}

int PROT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 7;
	*firer = 250;
	*fireg = 170;
	*fireb = 170;

	*pixel_mode |= FIRE_BLEND;
	return 1;
}

void PROT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = RNG::Ref().between(0, 35) * 0.17453f;
	sim->parts[i].life = 680;
	sim->parts[i].vx = 2.0f * cosf(a);
	sim->parts[i].vy = 2.0f * sinf(a);
}

void PROT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PROT";
	elem->Name = "PROT";
	elem->Colour = COLPACK(0x990000);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 40;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = -1;

	elem->HeatConduct = 61;
	elem->Latent = 0;
	elem->Description = "Protons. Transfer heat to materials, and removes sparks.";

	elem->Properties = TYPE_ENERGY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 75;

	elem->Update = &PROT_update;
	elem->Graphics = &PROT_graphics;
	elem->Func_Create = &PROT_create;
	elem->Init = &PROT_init_element;
}
