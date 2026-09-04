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

int PLUT_update(UPDATE_FUNC_ARGS)
{
	if (RNG::Ref().chance(1, 100) && RNG::Ref().chance(5.0f * sim->air->pv[y/CELL][x/CELL], 1000))
	{
		sim->part_create(i, x, y, PT_NEUT);
	}
	return 0;
}

void PLUT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PLUT";
	elem->Name = "PLUT";
	elem->Colour = COLPACK(0x407020);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.4f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;
	elem->PhotonReflectWavelengths = 0x001FCE00;

	elem->Weight = 90;

	elem->DefaultProperties.temp = R_TEMP + 4.0f + 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "钚。重的裂变粒子。在压力下产生中子。";
	elem->DetailedDescription = "描述：裂变，在高压下、被闪电击中或大量的中子(NEUT)轰击时更不稳定。反应产物是铀(URAN)、中子(NEUT)、熔融态的钚(熔融 PLUT)，并带来最高的温度和少量的火焰。如果压力低于-2 P，PLUT 将不会与中子反应。冷却之后将会形成石粉(STNE)。会杀死火柴人(STKM)。当被白光照射时，PLUT 将大部分反射为绿光，光谱中带有微小的黄线和蓝线。\n导热率：251\n初始温度：26.00℃/299.15K";

	elem->Properties = TYPE_PART|PROP_NEUTPASS|PROP_RADIOACTIVE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PLUT_update;
	elem->Graphics = NULL;
	elem->Init = &PLUT_init_element;
}
