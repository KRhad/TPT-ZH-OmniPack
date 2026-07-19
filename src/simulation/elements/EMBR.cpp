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

int EMBR_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((sim->elements[TYP(r)].Properties & (TYPE_SOLID | TYPE_PART | TYPE_LIQUID)) && !(sim->elements[TYP(r)].Properties & PROP_SPARKSETTLE))
				{
					sim->part_kill(i);
					return 1;
				}
			}
	return 0;
}

int EMBR_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->ctype & 0xFFFFFF)
	{
		*colr = (cpart->ctype & 0xFF0000)>>16;
		*colg = (cpart->ctype & 0x00FF00)>>8;
		*colb = (cpart->ctype & 0x0000FF);

		int maxComponent = *colr;
		if (*colg > maxComponent)
			maxComponent = *colg;
		if (*colb > maxComponent)
			maxComponent = *colb;

		// Make sure it isn't too dark to see
		if (maxComponent < 60)
		{
			float multiplier = 60.0f / maxComponent;
			*colr = (int)(*colr*multiplier);
			*colg = (int)(*colg*multiplier);
			*colb = (int)(*colb*multiplier);
		}
	}
	else if (cpart->tmp != 0)
	{
		*colr = *colg = *colb = 255;
	}

	bool deco = false;
	if (decorations_enable && cpart->dcolour && (cpart->dcolour&0xFF000000))
	{
		// commented out because no "blackDecorations" in my mod
		/*if (!blackDecorations) // if blackDecorations is off, always show deco
			deco = true;
		else/ if (((cpart->dcolour>>24)&0xFF) >= 250 && ((cpart->dcolour>>16)&0xFF) <= 5 && ((cpart->dcolour>>8)&0xFF) <= 5 && ((cpart->dcolour)&0xFF) <= 5) // else only render black deco*/
			deco = true;
	}

	if (deco)
	{
		int a = COLA(cpart->dcolour);
		*colr = (a*COLR(cpart->dcolour) + (255-a)**colr) >> 8;
		*colg = (a*COLG(cpart->dcolour) + (255-a)**colg) >> 8;
		*colb = (a*COLB(cpart->dcolour) + (255-a)**colb) >> 8;
	}
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	if (cpart->tmp == 1)
	{
		*pixel_mode = FIRE_ADD | PMODE_BLEND | PMODE_GLOW;
		*firea = (cpart->life - 15) * 4;
		*cola = (cpart->life + 15) * 4;
	}
	else if (cpart->tmp == 2)
	{
		*pixel_mode = PMODE_FLAT | FIRE_ADD;
		*firea = 255;
	}
	else
	{
		*pixel_mode = PMODE_SPARK | PMODE_ADD;
		if (cpart->life < 64)
			*cola = 4 * cpart->life;
	}
	return 0;
}

void EMBR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_EMBR";
	elem->Name = "EMBR";
	elem->Colour = COLPACK(0xFFF288);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.001f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.90f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.07f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 21;

	elem->Weight = 30;

	elem->DefaultProperties.temp = 500.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Sparks. Formed by explosions.";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL|PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 50;

	elem->Update = &EMBR_update;
	elem->Graphics = &EMBR_graphics;
	elem->Init = &EMBR_init_element;
}
