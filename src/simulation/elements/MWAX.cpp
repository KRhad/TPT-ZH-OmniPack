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

void MWAX_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_MWAX";
	elem->Name = "MWAX";
	elem->Colour = COLPACK(0xE0E0AA);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.3f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.15f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000001f* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 5;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 2;

	elem->Weight = 25;

	elem->DefaultProperties.temp = R_TEMP + 28.0f + 273.15f;
	elem->HeatConduct = 44;
	elem->Latent = 0;
	elem->Description = "液体蜡。 45度时硬化成WAX。";
	elem->DetailedDescription = "描述：融化的蜡(WAX)，可以燃烧，45℃时凝固成蜡(WAX)。蜡油不是非常易燃，除非在高温(673K 以上)下会导致蜡油立即点燃。\n燃点：399.85℃/673K\n凝固点：44.85℃/318K\n导热率：44\n初始温度：50.00℃/323.15K";

	elem->Properties = TYPE_LIQUID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 318.0f;
	elem->LowTemperatureTransitionElement = PT_WAX;
	elem->HighTemperatureTransitionThreshold = 673.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Init = &MWAX_init_element;
}
