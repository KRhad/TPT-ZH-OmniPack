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

void PLEX_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PLEX";
	elem->Name = "C-4";
	elem->Colour = COLPACK(0xD080E0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
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

	elem->Flammable = 1000;
	elem->Explosive = 2;
	elem->Meltable = 50;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x1F00003E;

	elem->Weight = 100;

	elem->HeatConduct = 88;
	elem->Latent = 0;
	elem->Description = "固体压敏炸药。";
	elem->DetailedDescription = "爆炸点：399.85℃/673K\n描述：压力敏感型炸药，暴露在高压(3 P 左右)下、电脉冲或者达到爆炸点都可以引发爆炸。暴露在中子\n(NEUT)下会变成粘土(GOO)。\n导热率：88\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

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
	elem->Init = &PLEX_init_element;
}
