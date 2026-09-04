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

#include "common/tpt-math.h"
#include "simulation/ElementsCommon.h"

int RPEL_update(UPDATE_FUNC_ARGS)
{
	for (int ri = 0; ri <= 10; ri++)
	{
		int rx = RNG::Ref().between(-10, 10);
		int ry = RNG::Ref().between(-10, 10);
		if (x+rx >= 0 && x+rx < XRES && y+ry >= 0 && y+ry < YRES && (rx || ry))
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				r = photons[y+ry][x+rx];

			if (r && !(sim->elements[TYP(r)].Properties & TYPE_SOLID))
			{
				if (!parts[i].ctype || parts[i].ctype == parts[ID(r)].type)
				{
					parts[ID(r)].vx = restrict_flt(parts[ID(r)].vx + isign<int>(rx) * ((parts[i].temp - 273.15) / 10.0f), -MAX_VELOCITY, MAX_VELOCITY);
					parts[ID(r)].vy = restrict_flt(parts[ID(r)].vy + isign<int>(ry) * ((parts[i].temp - 273.15) / 10.0f), -MAX_VELOCITY, MAX_VELOCITY);
				}
			}
		}
	}
	return 0;
}

void RPEL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RPEL";
	elem->Name = "RPEL";
	elem->Colour = COLPACK(0x99CC00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
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

	elem->DefaultProperties.temp = 20.0f + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "根据温度排斥或吸引粒子。";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &RPEL_update;
	elem->Graphics = NULL;
	elem->CtypeDraw = &basicCtypeDraw;
	elem->Init = &RPEL_init_element;
}
