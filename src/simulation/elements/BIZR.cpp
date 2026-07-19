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

#define BLEND 21.0f

int BIZR_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].dcolour)
	{
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (TYP(r)!=PT_BIZR && TYP(r)!=PT_BIZRG  && TYP(r)!=PT_BIZRS)
					{
						int tr = (parts[ID(r)].dcolour >> 16) & 0xFF;
						int tg = (parts[ID(r)].dcolour >> 8 ) & 0xFF;
						int tb = (parts[ID(r)].dcolour      ) & 0xFF;
						int ta = (parts[ID(r)].dcolour >> 24) & 0xFF;

						int mr = (parts[i].dcolour >> 16) & 0xFF;
						int mg = (parts[i].dcolour >> 8 ) & 0xFF;
						int mb = (parts[i].dcolour      ) & 0xFF;
						int ma = (parts[i].dcolour >> 24) & 0xFF;

						int nr = tr + ((mr > tr) - (mr < tr)) + int(std::round((mr - tr) / BLEND));
						int ng = tg + ((mg > tg) - (mg < tg)) + int(std::round((mg - tg) / BLEND));
						int nb = tb + ((mb > tb) - (mb < tb)) + int(std::round((mb - tb) / BLEND));
						int na = ta + ((ma > ta) - (ma < ta)) + int(std::round((ma - ta) / BLEND));

						parts[ID(r)].dcolour = COLARGB(na, nr, ng, nb);
					}
				}
	}
	return 0;
}

int BIZR_graphics(GRAPHICS_FUNC_ARGS)
{
	int x = 0;
	float brightness = fabs(cpart->vx) + fabs(cpart->vy);
	if (cpart->ctype&0x3FFFFFFF)
	{
		*colg = 0;
		*colb = 0;
		*colr = 0;
		for (x=0; x<12; x++) {
			*colr += (cpart->ctype >> (x+18)) & 1;
			*colb += (cpart->ctype >>  x)     & 1;
		}
		for (x=0; x<12; x++)
			*colg += (cpart->ctype >> (x+9))  & 1;
		x = 624 / (*colr + *colg + *colb + 1);
		*colr *= x;
		*colg *= x;
		*colb *= x;
	}
	if (brightness > 0)
	{
		brightness /= 5;
		*firea = 255;
		*firer = (int)(*colr * brightness);
		*fireg = (int)(*colg * brightness);
		*fireb = (int)(*colb * brightness);
		*pixel_mode |= FIRE_ADD;
	}
	*pixel_mode |= PMODE_BLUR;
	return 0;
}

void BIZR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BIZR";
	elem->Name = "BIZR";
	elem->Colour = COLPACK(0x00FF77);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Bizarre... contradicts the normal state changes. Paints other elements with its deco color.";

	elem->Properties = TYPE_LIQUID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 100.0f;
	elem->LowTemperatureTransitionElement = PT_BIZRG;
	elem->HighTemperatureTransitionThreshold = 400.0f;
	elem->HighTemperatureTransitionElement = PT_BIZRS;

	elem->DefaultProperties.ctype = 0x47FFFF;

	elem->Update = &BIZR_update;
	elem->Graphics = &BIZR_graphics;
	elem->Init = &BIZR_init_element;
}
