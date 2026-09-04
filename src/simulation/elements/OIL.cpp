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

void OIL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_OIL";
	elem->Name = "OIL";
	elem->Colour = COLPACK(0x404010);
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

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 5;

	elem->Weight = 20;

	elem->HeatConduct = 42;
	elem->Latent = 0;
	elem->Description = "易燃，低压或高温下转为气体";

	elem->Properties = TYPE_LIQUID|PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 333.0f;
	elem->HighTemperatureTransitionElement = PT_GAS;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &OIL_init_element;
}
