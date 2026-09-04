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

int QRTZ_update(UPDATE_FUNC_ARGS);
int QRTZ_graphics(GRAPHICS_FUNC_ARGS);

void PQRT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp2 = RNG::Ref().between(0, 10);
}

void PQRT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PQRT";
	elem->Name = "PQRT";
	elem->Colour = COLPACK(0x88BBBB);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.27f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 90;

	elem->HeatConduct = 3;
	elem->Latent = 0;
	elem->Description = "石英粉，QRTZ 的破碎形式。";
	elem->DetailedDescription = "描述：石英砂，可以熔化。QRTZ 的破碎形式。可以通过从底部缓慢加热来熔炼回石英。如果放入 SLTW，可以将其转换 QRTZ。\n反应：DMG + QRTZ → PQRT\nCLST( 熔融) + PQRT(熔融) → 2×CRMC(熔融)\n熔点：2300℃/2573.15K\n导热率：3\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_PART | PROP_PHOTPASS | PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2573.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &QRTZ_update;
	elem->Graphics = &QRTZ_graphics;
	elem->Func_Create = &PQRT_create;
	elem->Init = &PQRT_init_element;
}
