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

int GBMB_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life <= 0)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
			{
				int r = pmap[y+ry][x+rx];
				if(!r)
					continue;
				if (TYP(r) !=PT_BOMB && TYP(r) != PT_GBMB && !(sim->elements[TYP(r)].Properties & PROP_CLONE)
				        && !(sim->elements[TYP(r)].Properties & PROP_INDESTRUCTIBLE))
				{
					parts[i].life = 60;
					break;
				}
			}
	}
	if (parts[i].life > 20)
		sim->grav->gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = 20;
	else if (parts[i].life >= 1)
		sim->grav->gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = -80;
	return 0;
}

int GBMB_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life <= 0)
		*pixel_mode |= PMODE_FLARE;
	else
		*pixel_mode |= PMODE_SPARK;
	return 0;
}

void GBMB_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GBMB";
	elem->Name = "GBMB";
	elem->Colour = COLPACK(0x1144BB);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "引力炸弹。黏在最先接触的物体上，随后产生强大斥力。";
	elem->DetailedDescription = "描述：引力炸弹是一种非常独特的“炸药”。它是游戏中唯一在引爆时不会产生热效应的炸药(DMG 某些情况下除外)。它在爆发出强烈的正重力(拉力效应)，然后是负重力(推力效应)。这些力足以破坏一些脆性固体，例如GLAS和 BMTL。GBMB 离其他粒子越近，效果越强，但它仍然对整个屏幕产生影响，但离它越远的东西就越少。重力冲击波可以影响能量类粒子在屏幕上的路径。\n爆炸过程：碰触物质后 Life 值变为 60，发光并附着在物质上，产生引力(20)Life 值降至 20 以下时，瞬间改变周围引力值为-80。\n导热率：251\n初始温度：20.00℃/292.15K";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &GBMB_update;
	elem->Graphics = &GBMB_graphics;
	elem->Init = &GBMB_init_element;
}
