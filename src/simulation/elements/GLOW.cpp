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

int GLOW_update(UPDATE_FUNC_ARGS)
{
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				if (TYP(r) == PT_WATR && RNG::Ref().chance(1, 400))
				{
					sim->part_kill(i);
					part_change_type(ID(r), x+rx, y+ry, PT_DEUT);
					parts[ID(r)].life = 10;

					return 1;
				}
				else if (TYP(r) == PT_GEL) //GLOW + GEL = RSST
				{
					sim->part_kill(i);
					sim->part_change_type(ID(r),x+rx,y+ry,PT_RSST);
					parts[ID(r)].tmp = 0;

					return 1;
				}
			}
	int ctype = (int)(sim->air->pv[y/CELL][x/CELL]*16);
	if (ctype < 0)
		ctype = 0;
	parts[i].ctype = ctype;
	parts[i].tmp = abs((int)((sim->air->vx[y/CELL][x/CELL]+sim->air->vy[y/CELL][x/CELL])*16.0f)) + abs((int)((parts[i].vx+parts[i].vy)*64.0f));
	return 0;
}

int GLOW_graphics(GRAPHICS_FUNC_ARGS)
{
	*firer = 16+int(restrict_flt(cpart->temp-(273.15f+34.0f), 0, 128)/2.0f);
	*fireg = 16+int(restrict_flt(float(cpart->ctype), 0, 128)/2.0f);
	*fireb = 16+int(restrict_flt(float(cpart->tmp), 0, 128)/2.0f);
	*firea = 64;

	*colr = int(restrict_flt(64.0f+cpart->temp-(273.15f+34.0f), 0, 255));
	*colg = int(restrict_flt(64.0f+cpart->ctype, 0, 255));
	*colb = int(restrict_flt(64.0f+cpart->tmp, 0, 255));

	int rng = RNG::Ref().between(1, 32); //
	if(((*colr) + (*colg) + (*colb)) > (256 + rng)) {
		*colr -= 54;
		*colg -= 54;
		*colb -= 54;
		*pixel_mode |= FIRE_ADD;
		*pixel_mode |= PMODE_GLOW | PMODE_ADD;
		*pixel_mode &= ~PMODE_FLAT;
	} else {
		*pixel_mode |= PMODE_BLUR;
	}
	return 0;
}

void GLOW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GLOW";
	elem->Name = "GLOW";
	elem->Colour = COLPACK(0x445464);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.3f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.15f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 2;

	elem->Weight = 40;

	elem->DefaultProperties.temp = R_TEMP + 20.0f + 273.15f;
	elem->HeatConduct = 44;
	elem->Latent = 0;
	elem->Description = "荧光液。受压时发光。";
	elem->DetailedDescription = "描述：荧光液，状态、压力或温度变化时改变颜色，与水混合产生重水(DEUT)。光子(PHOT)接触到它会增殖。可以使电子(ELEC)变成光子(PHOT)，但仅限于 GLOW 内部。\n颜色表：\n颜色状态灰色正常蓝色移动中亮红色高温深绿/深蓝低温翠绿低压黄色高温高压亮粉高温低压暗一些的翠绿低温高压深蓝低温低压\n导热率：44\n初始温度：42.00℃/315.15K";

	elem->Properties = TYPE_LIQUID | PROP_PHOTPASS | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &GLOW_update;
	elem->Graphics = &GLOW_graphics;
	elem->Init = &GLOW_init_element;
}
