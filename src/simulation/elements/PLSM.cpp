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
#include "graphics.h"

int FIRE_update(UPDATE_FUNC_ARGS);

int PLSM_graphics(GRAPHICS_FUNC_ARGS)
{
	int caddress = (int)restrict_flt(restrict_flt((float)cpart->life, 0.0f, 200.0f)*3, 0.0f, (200.0f*3)-3);
	*colr = (unsigned char)plasma_data[caddress];
	*colg = (unsigned char)plasma_data[caddress+1];
	*colb = (unsigned char)plasma_data[caddress+2];
	
	*firea = 255;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	
	*pixel_mode = PMODE_GLOW | PMODE_ADD; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_ADD;
	//Returning 0 means dynamic, do not cache
	return 0;
}

void PLSM_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = RNG::Ref().between(50, 199);
}

void PLSM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PLSM";
	elem->Name = "PLSM";
	elem->Colour = COLPACK(0xBB99FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 0.9f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.20f;
	elem->Collision = 0.0f;
	elem->Gravity = -0.1f;
	elem->Diffusion = 0.30f;
	elem->HotAir = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = MAX_TEMP;
	elem->HeatConduct = 5;
	elem->Latent = 0;
	elem->Description = "等离子体。极热。";

	elem->Properties = TYPE_GAS | PROP_LIFE_DEC;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FIRE_update;
	elem->Graphics = &PLSM_graphics;
	elem->Func_Create = &PLSM_create;
	elem->Init = &PLSM_init_element;
}
