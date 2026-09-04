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

int EXPL_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (!(sim->elements[TYP(r)].Properties & PROP_INDESTRUCTIBLE) && TYP(r) != PT_EMBR)
				{
					parts[ID(r)].flags |= FLAG_EXPLODE;
				}
			}
	return 0;
}

int EXPL_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_FLARE;
	return 0;
}

void EXPL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_EXPL";
	elem->Name = "EXPL";
	elem->Colour = COLPACK(0xFEA713);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.10f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 99;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "爆炸，会使接触到的一切发生爆炸。";
	elem->DetailedDescription = "描述：连锁爆炸粒子，是会移动的高亮粉末。它检查周围 3×3 范围，并给所有非不可破坏、非 EMBR 的相邻粒子设置爆炸标记，使这些粒子按各自的爆炸规则被摧毁或引爆。\n特性：EXPL 自身具有不可破坏属性，不靠燃烧或倒计时工作，因此一小团即可沿可破坏材料持续传播爆炸；不可破坏材料能阻止传播。它不会把余烬(EMBR)标记为爆炸。\n导热率：29\n初始温度：20℃/293.15K";

	elem->Properties = TYPE_PART|PROP_SPARKSETTLE|PROP_INDESTRUCTIBLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &EXPL_update;
	elem->Graphics = &EXPL_graphics;
	elem->Init = &EXPL_init_element;
}
