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
#include "simulation/GolNumbers.h"

int PCLN_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt;
	if (parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !sim->elements[parts[i].ctype].Enabled)
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK)
				{
					r = photons[y+ry][x+rx];
					if (!r)
						r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					rt = TYP(r);
					if (!(sim->elements[rt].Properties & PROP_CLONE) &&
						!(sim->elements[rt].Properties & PROP_BREAKABLECLONE) &&
				        rt != PT_SPRK && rt != PT_NSCN && 
						rt != PT_PSCN && rt != PT_STKM && 
						rt != PT_STKM2)
					{
						parts[i].ctype = TYP(r);
						if (rt==PT_LIFE || rt==PT_LAVA)
							parts[i].tmp = parts[ID(r)].ctype;
					}
				}
	if (parts[i].ctype>0 && parts[i].ctype<PT_NUM && sim->elements[parts[i].ctype].Enabled && parts[i].life==10)
	{
		//create photons a different way
		if (parts[i].ctype == PT_PHOT)
		{
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (rx || ry)
					{
						int r = sim->part_create(-1, x+rx, y+ry, PT_PHOT);
						if (r != -1)
						{
							parts[r].vx = rx*3.0f;
							parts[r].vy = ry*3.0f;
							if (r>i)
							{
								// Make sure movement doesn't happen until next frame, to avoid gaps in the beams of photons produced
								parts[r].flags |= FLAG_SKIPMOVE;
							}
						}
					}
		}
		//create life a different way
		else if (parts[i].ctype == PT_LIFE)
		{
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
				{
					sim->part_create(-1, x+rx, y+ry, PT_LIFE, parts[i].tmp);
				}
		}
		else if (parts[i].ctype != PT_LIGH || RNG::Ref().chance(1, 30))
		{
			int np = sim->part_create(-1, x + RNG::Ref().between(-1, 1), y + RNG::Ref().between(-1, 1), TYP(parts[i].ctype));
			if (np >= 0)
			{
				if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && sim->elements[parts[i].tmp].HighTemperatureTransitionElement==PT_LAVA)
					parts[np].ctype = parts[i].tmp;
			}
		}
	}
	return 0;
}

int PCLN_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod = ((cpart->life>10?10:cpart->life)*10);
	*colr += lifemod;
	*colg += lifemod;
	return 0;
}

bool PCLN_ctypeDraw(CTYPEDRAW_FUNC_ARGS)
{
	if (t == PT_PSCN || t == PT_NSCN || t == PT_SPRK)
		return false;
	return ctypeDrawVInTmp(CTYPEDRAW_FUNC_SUBCALL_ARGS);
}

void PCLN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PCLN";
	elem->Name = "PCLN";
	elem->Colour = COLPACK(0x3B3B0A);
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
	elem->Description = "Powered clone. When activated, duplicates any particles it touches.";

	elem->Properties = TYPE_SOLID | PROP_CLONE | PROP_POWERED | PROP_NOCTYPEDRAW;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &PCLN_graphics;
	elem->CtypeDraw = &PCLN_ctypeDraw;
	elem->Init = &PCLN_init_element;
}
