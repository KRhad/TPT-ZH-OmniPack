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

int RFRG_update(UPDATE_FUNC_ARGS);

void RFGL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_RFGL";
	elem->Name = "RFGL";
	elem->Colour = PIXPACK(0x84C2CF);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f * CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 21;

	elem->Weight = 10;

	elem->HeatConduct = 3;
	elem->Description = "液体制冷剂。";
	elem->DetailedDescription = "描述：液态制冷剂，是 RFRG 在较高压力下形成的液相。压力低于 2 P 时转回气态 RFRG；气态 RFRG 压力升到 2 P 时凝结为 RFGL，由此可构成压缩—膨胀制冷循环。\n热力过程：RFRG 会按新旧绝对压力比例改变温度，近似公式为 T新=T旧×(P新+257)/(P旧+257)：压缩升温、膨胀降温。RFGL/RFRG 导热很慢，便于把温差带到换热端。\n反应：中子撞击气态 RFRG 时，等概率转成 GAS 或 CAUS。RFGL 和 RFRG 都具有致命属性，火柴人应避免接触。\n导热率：3\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_LIQUID|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = 2;
	elem->LowPressureTransitionElement = PT_RFRG;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &RFRG_update;
}
