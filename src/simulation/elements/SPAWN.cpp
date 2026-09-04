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
#include "simulation/elements/STKM.h"

bool SPAWN_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1()->spawnID == -1;
}

void SPAWN_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to == PT_SPAWN)
	{
		Stickman *player = static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1();
		if (player->spawnID == -1)
			player->spawnID = i;
	}
	else
	{
		Stickman *player = static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1();
		if (player->spawnID == i)
			player->spawnID = -1;
	}
}

void SPAWN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SPAWN";
	elem->Name = "SPWN";
	elem->Colour = COLPACK(0xAAAAAA);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "STKM 生成点。";
	elem->DetailedDescription = "描述：受重力和压力影响，使用方向键来控制其运动，可以在水下呼吸，受到压力、高温、放射性物质等各种危险的东西会损失生命值，吃(走近)植物(PLNT)可以恢复生命值，修改 Life 值可以修改生命值上限，以下\n能力：\n复制：火柴人可以复制他的头碰到的物质，当碰到一个特定物质(或墙)时，他的头会改变颜色，此时按方向键↓，火柴人就会吐出该种物质。\n使用电子产品：当火柴人的头碰到金属时，按方向键↓可以给金属一个电脉冲，这样就可以使用电子产品了。\n火箭鞋：当火柴人碰到重力墙时，会拥有火箭鞋(喷出高温的等离子体 PLSM)，同样用方向键控制，碰到电锁体(E-Hole)时会恢复原状。\n导热率：0/0\n燃点：346.85℃/620K\n初始温度：36.6℃/309.75K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Func_Create_Allowed = &SPAWN_create_allowed;
	elem->Func_ChangeType = &SPAWN_ChangeType;
	elem->Init = &SPAWN_init_element;
}
