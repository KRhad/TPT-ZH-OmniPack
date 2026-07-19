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

int DMG_update(UPDATE_FUNC_ARGS)
{
	const int rad = 25;

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) != PT_DMG && TYP(r) != PT_EMBR && TYP(r) != PT_DMND && TYP(r) != PT_CLNE && TYP(r) != PT_PCLN && TYP(r) != PT_BCLN)
				{
					sim->part_kill(i);
					for (int nxj = -rad; nxj <= rad; nxj++)
						for (int nxi = -rad; nxi <= rad; nxi++)
							if (x+nxi>=0 && y+nxj>=0 && x+nxi<XRES && y+nxj<YRES && (nxi || nxj))
							{
								int dist = (int)(sqrt(pow((float)nxi, 2.0f)+pow((float)nxj, 2.0f)));
								if (!dist || (dist <= rad))
								{
									int rr = pmap[y+nxj][x+nxi];
									if (rr)
									{
										float angle = atan2((float)nxj, (float)nxi);
										float fx = cos(angle) * 7.0f;
										float fy = sin(angle) * 7.0f;

										parts[ID(rr)].vx += fx;
										parts[ID(rr)].vy += fy;

										sim->air->vx[(y+nxj)/CELL][(x+nxi)/CELL] += fx;
										sim->air->vy[(y+nxj)/CELL][(x+nxi)/CELL] += fy;
										sim->air->pv[(y+nxj)/CELL][(x+nxi)/CELL] += 1.0f;
										
										int t = TYP(rr);
										if (t && sim->elements[t].HighPressureTransitionThreshold>-1 && sim->elements[t].HighPressureTransitionThreshold<PT_NUM)
											sim->part_change_type(ID(rr), x+nxi, y+nxj, sim->elements[t].HighPressureTransitionElement);
										else if (t == PT_BMTL)
											sim->part_change_type(ID(rr), x+nxi, y+nxj, PT_BRMT);
										else if (t == PT_GLAS)
											sim->part_change_type(ID(rr), x+nxi, y+nxj, PT_BGLA);
										else if (t == PT_COAL)
											sim->part_change_type(ID(rr), x+nxi, y+nxj, PT_BCOL);
										else if (t == PT_QRTZ)
											sim->part_change_type(ID(rr), x+nxi, y+nxj, PT_PQRT);
										else if (t == PT_TUNG)
										{
											sim->part_change_type(ID(rr), x+nxi, y+nxj, PT_BRMT);
											parts[ID(rr)].ctype = PT_TUNG;
										}
										else if (t == PT_WOOD)
											sim->part_change_type(ID(rr), x+nxi, y+nxj, PT_SAWD);
									}
								}
							}
					return 1;
				}
			}
	return 0;
}

int DMG_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_FLARE;
	return 1;
}

void DMG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DMG";
	elem->Name = "DMG";
	elem->Colour = COLPACK(0x88FF88);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0;
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
	elem->Description = "Generates damaging pressure and breaks any elements it hits.";

	elem->Properties = TYPE_PART|PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DMG_update;
	elem->Graphics = &DMG_graphics;
	elem->Init = &DMG_init_element;
}
