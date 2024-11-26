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

int ICE_update(UPDATE_FUNC_ARGS);

void SNOW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SNOW";
	elem->Name = "SNOW";
	elem->Colour = COLPACK(0xC0E0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.90f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.05f;
	elem->Diffusion = 0.01f;
	elem->HotAir = -0.00005f* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;
	elem->PhotonReflectWavelengths = 0x03FFFFFF;

	elem->Weight = 50;

	elem->DefaultProperties.temp = R_TEMP - 30.0f + 273.15f;
	elem->HeatConduct = 46;
	elem->Latent = 1095;
	elem->Description = "Light particles. Created when ICE breaks under pressure.";

	elem->Properties = TYPE_PART|PROP_NEUTPASS;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 252.05f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &ICE_update;
	elem->Graphics = NULL;
	elem->Init = &SNOW_init_element;
}
