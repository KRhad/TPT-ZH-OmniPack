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

int LCRY_update(UPDATE_FUNC_ARGS)
{
	int check, setto;
	switch (parts[i].tmp)
	{
	case 1:
		if(parts[i].life<=0)
			parts[i].tmp = 0;
		else
		{
			parts[i].life-=2;
			if(parts[i].life < 0)
				parts[i].life = 0;
			parts[i].tmp2 = parts[i].life;
		}
	case 0:
		check=3;
		setto=1;
		break;
	case 2:
		if(parts[i].life>=10)
			parts[i].tmp = 3;
		else
		{
			parts[i].life+=2;
			if(parts[i].life > 10)
				parts[i].life = 10;
			parts[i].tmp2 = parts[i].life;
		}
	case 3:
		check=0;
		setto=2;
		break;
	default:
		parts[i].tmp = 0;
		parts[i].life = 0;
		return 0;
	}
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_LCRY && parts[ID(r)].tmp == check)
				{
					parts[ID(r)].tmp = setto;
				}
			}
	return 0;
}

int LCRY_graphics(GRAPHICS_FUNC_ARGS)
{
	if(decorations_enable && cpart->dcolour && COLA(cpart->dcolour))
	{
		*colr = COLR(cpart->dcolour);
		*colg = COLG(cpart->dcolour);
		*colb = COLB(cpart->dcolour);

		if(cpart->tmp2<10){
			*colr /= 10-cpart->tmp2;
			*colg /= 10-cpart->tmp2;
			*colb /= 10-cpart->tmp2;
		}
		
	}
	else
	{
		*colr = *colg = *colb = 0x50+((cpart->tmp2>10?10:cpart->tmp2)*10);
	}
	*pixel_mode |= NO_DECO;
	return 0;
					
	/*int lifemod = ((cpart->tmp2>10?10:cpart->tmp2)*10);
	*colr += lifemod; 
	*colg += lifemod; 
	*colb += lifemod; 
	if(decorations_enable && cpart->dcolour && cpart->dcolour&0xFF000000)
	{
		lifemod *= 2.5f;
		if(lifemod < 40)
			lifemod = 40;
		*colr = (lifemod*((cpart->dcolour>>16)&0xFF) + (255-lifemod)**colr) >> 8;
		*colg = (lifemod*((cpart->dcolour>>8)&0xFF) + (255-lifemod)**colg) >> 8;
		*colb = (lifemod*((cpart->dcolour)&0xFF) + (255-lifemod)**colb) >> 8;
	}
	*pixel_mode |= NO_DECO;
	return 0;*/
}

void LCRY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LCRY";
	elem->Name = "LCRY";
	elem->Colour = COLPACK(0x505050);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
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

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "液晶。充电时会改变颜色。 （PSCN 充电，NSCN 放电）";
	elem->DetailedDescription = "描述：液晶，激活时改变颜色。用P 型硅(PSCN)激活，N 型硅(NSCN)关闭。激活时允许光子(PHOT)缓慢通过，极高温度(999.85℃/1273K)下会变成碎玻璃(BGLA)。激活后，部分通过它的质子(PROT)会被转变为光子(PHOT)。\n导热率：251\n初始温度：22.00℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1273.0f;
	elem->HighTemperatureTransitionElement = PT_BGLA;

	elem->Update = &LCRY_update;
	elem->Graphics = &LCRY_graphics;
	elem->Init = &LCRY_init_element;
}
