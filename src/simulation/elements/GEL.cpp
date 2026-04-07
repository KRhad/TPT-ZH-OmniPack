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

int GEL_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp>100) parts[i].tmp = 100;
	if (parts[i].tmp<0) parts[i].tmp = 0;
	int absorbChanceDenom = parts[i].tmp*10 + 500;
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rt = TYP(r);

				//Desaturation
				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
				case PT_FRZW:
					if (parts[i].tmp<100 && RNG::Ref().chance(500, absorbChanceDenom))
					{
						parts[i].tmp++;
						sim->part_kill(ID(r));
					}
					break;
				case PT_PSTE:
					if (parts[i].tmp<100 && RNG::Ref().chance(20, absorbChanceDenom))
					{
						parts[i].tmp++;
						sim->part_create(ID(r), x+rx, y+ry, PT_CLST);
					}
					break;
				case PT_SLTW:
					if (parts[i].tmp<100 && RNG::Ref().chance(50, absorbChanceDenom))
					{
						parts[i].tmp++;
						if (RNG::Ref().chance(3, 4))
							sim->part_kill(ID(r));
						else
							part_change_type(ID(r), x+rx, y+ry, PT_SALT);
					}
					break;
				case PT_CBNW:
					if (parts[i].tmp<100 && RNG::Ref().chance(100, absorbChanceDenom))
					{
						parts[i].tmp++;
						part_change_type(ID(r), x+rx, y+ry, PT_CO2);
					}
					break;
				case PT_SPNG:
					// Concentration diffusion
					if (parts[ID(r)].life>0 && parts[i].tmp<100 && ((parts[ID(r)].life+1)>parts[i].tmp))
					{
						// SPNG -> GEL
						parts[ID(r)].life--;
						parts[i].tmp++;
					}
					else if (parts[i].tmp>0 && parts[ID(r)].life+1 < parts[i].tmp)
					{
						// SPNG <- GEL (saturation limit of SPNG is ignored here)
						parts[ID(r)].life++;
						parts[i].tmp--;
					}
					break;
				case PT_GEL:
					//Concentration diffusion
					if ((parts[ID(r)].tmp+1) < parts[i].tmp)
					{
						parts[ID(r)].tmp++;
						parts[i].tmp--;
					}
					break;
				default:
					break;
				}

				float dx = parts[i].x - parts[ID(r)].x;
				float dy = parts[i].y - parts[ID(r)].y;

				//Stickness
				if ((dx*dx + dy*dy)>1.5 && (rt == PT_GEL || !sim->elements[rt].Falldown || (fabs((float)rx)<2 && fabs((float)ry)<2)))
				{
					float per, nd = dx*dx + dy*dy - 0.5f;

					per = 5*(1 - parts[i].tmp/100)*(nd/(dx*dx + dy*dy + nd) - 0.5f);
					if (sim->elements[TYP(r)].Properties&TYPE_LIQUID)
						per *= 0.1f;
					
					dx *= per; dy *= per;
					parts[i].vx += dx; 
					parts[i].vy += dy; 

					if (sim->elements[rt].Properties&TYPE_PART || rt==PT_GOO)
					{
						parts[ID(r)].vx -= dx;
						parts[ID(r)].vy -= dy;
					}
				}
			}
	return 0;
}

int GEL_graphics(GRAPHICS_FUNC_ARGS)
{
	int q = cpart->tmp;
	*colr = q*(32-255)/120+255;
	*colg = q*(48-186)/120+186;
	*colb = q*208/120;
	*pixel_mode |= PMODE_BLUR;
	return 0;
}

void GEL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GEL";
	elem->Name = "GEL";
	elem->Colour = COLPACK(0xFF9900);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 35;

	elem->DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "A liquid with variable viscosity and heat conductivity. Absorbs water.";

	elem->Properties = TYPE_LIQUID|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &GEL_update;
	elem->Graphics = &GEL_graphics;
	elem->Init = &GEL_init_element;
}
