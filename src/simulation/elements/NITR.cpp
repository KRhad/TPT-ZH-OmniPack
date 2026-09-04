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

void NITR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_NITR";
	elem->Name = "NITR";
	elem->Colour = COLPACK(0x20E010);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.92f;
	elem->Loss = 0.97f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.2f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 1000;
	elem->Explosive = 2;
	elem->Meltable = 0;
	elem->Hardness = 3;
	elem->PhotonReflectWavelengths = 0x0007C000;

	elem->Weight = 23;

	elem->HeatConduct = 50;
	elem->Latent = 0;
	elem->Description = "硝酸甘油。压敏炸药。与CLST混合制成TNT。";
	elem->DetailedDescription = "燃点：399.85℃/673K\n描述：炸药，压力下(3 P 左右)、电脉冲、明火都可以引起爆炸。爆炸点与压力有关，压力越小爆炸点越低。\n暴露在中子下产生石油气(GAS)和柴油(DESL)。可以与粘土砂(CLST)混合形成三硝基甲苯(TNT)。如果用 NEUT 照射会转化为 OIL。\n导热率：50\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_LIQUID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 673.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &NITR_init_element;
}
