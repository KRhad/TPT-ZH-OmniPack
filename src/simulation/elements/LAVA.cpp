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

int FIRE_update(UPDATE_FUNC_ARGS);

int LAVA_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = cpart->life * 2 + 0xE0;
	*colg = cpart->life * 1 + 0x50;
	*colb = cpart->life / 2 + 0x10;
	if (*colr>255) *colr = 255;
	if (*colg>192) *colg = 192;
	if (*colb>128) *colb = 128;
	*firea = 40;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD;
	*pixel_mode |= PMODE_BLUR;
	//Returning 0 means dynamic, do not cache
	return 0;
}

void LAVA_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = RNG::Ref().between(240, 359);
}

void LAVA_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LAVA";
	elem->Name = "LAVA";
	elem->Colour = COLPACK(0xE05010);
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
	elem->HotAir = 0.0003f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 2;
	elem->PhotonReflectWavelengths = 0x3FF00000;

	elem->Weight = 45;

	elem->DefaultProperties.temp = R_TEMP + 1500.0f + 273.15f;
	elem->HeatConduct = 60;
	elem->Latent = 0;
	elem->Description = "熔岩。点燃易燃材料。金属和其他材料熔化时生成，冷时凝固。";
	elem->DetailedDescription = "描述：熔岩(LAVA)表示各种材料的熔融状态，外观相同，但 Ctype 记录原材料。冷却后通常恢复为 Ctype 对应的固体；核反应也可能生成熔融物。\nHUD 显示：尚未生成过的组合会显示为“熔融+材料名”。即使名称没有明确写出“熔岩”，粒子类型仍是 LAVA。用控制台改变 Ctype 可以制造熔融火柴人、熔融水等特殊组合，但这些组合不一定具有正常相变行为。\n高温可熔融物：除 BTRY、INST、WWLD 外的大多数电子元件；除 SNOW、BREL、ANAR、GRAV、FRZZ、BCOL、FSEP、YEST、DUST 外的大多数粉末；以及 BMTL、GLAS 等固体。\n导热率：60\n初始温度：1522℃/1795.15K";

	elem->Properties = TYPE_LIQUID|PROP_LIFE_DEC;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = MAX_TEMP; // check for lava solidification at all temperatures
	elem->LowTemperatureTransitionElement = ST;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FIRE_update;
	elem->Graphics = &LAVA_graphics;
	elem->Func_Create = &LAVA_create;
	elem->Init = &LAVA_init_element;
}
