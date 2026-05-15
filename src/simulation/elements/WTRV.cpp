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

int WTRV_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((TYP(r)==PT_RBDM||TYP(r)==PT_LRBD) && !legacy_enable && parts[i].temp>(273.15f+12.0f) && RNG::Ref().chance(1, 100))
				{
					part_change_type(i,x,y,PT_FIRE);
					parts[i].life = 4;
					parts[i].ctype = PT_WATR;
				}
			}
	if (parts[i].temp > 1273 && parts[i].ctype == PT_FIRE)
		parts[i].temp -= parts[i].temp / 1000;
	return 0;
}

void WTRV_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WTRV";
	elem->Name = "WTRV";
	elem->Colour = COLPACK(0xA0A0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = -0.1f;
	elem->Diffusion = 0.75f;
	elem->HotAir = 0.0003f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 4;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP + 100.0f + 273.15f;
	elem->HeatConduct = 48;
	elem->Latent = 0;
	elem->Description = "Steam. Produced from hot water.";

	elem->Properties = TYPE_GAS;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 371.0f;
	elem->LowTemperatureTransitionElement = ST;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WTRV_update;
	elem->Graphics = NULL;
	elem->Init = &WTRV_init_element;
}
