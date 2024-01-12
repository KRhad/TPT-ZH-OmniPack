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
#include "simulation/elements/FILT.h"

int ARAY_update(UPDATE_FUNC_ARGS)
{
	int short_bray_life = parts[i].life > 0 ? parts[i].life : 30;
	int long_bray_life = parts[i].life > 0 ? parts[i].life : 1020;
	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3)
				{
					bool isBlackDeco = false;
					int destroy = (parts[ID(r)].ctype==PT_PSCN) ? 1 : 0;
					int nostop = (parts[ID(r)].ctype==PT_INST) ? 1 : 0;
					int colored = 0, rt;
					for (int docontinue = 1, nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi)
					{
						if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0))
							break;

						r = pmap[y+nyi+nyy][x+nxi+nxx];
						rt = TYP(r);
						r = ID(r);
						if (!rt)
						{
							int nr = sim->part_create(-1, x+nxi+nxx, y+nyi+nyy, PT_BRAY);
							if (nr != -1)
							{
								// if it came from PSCN
								if (destroy)
								{
									parts[nr].tmp = 2;
									parts[nr].life = 2;
								}
								else
								{
									parts[nr].ctype = colored;
									parts[nr].life = short_bray_life;
								}
								parts[nr].temp = parts[i].temp;
								if (isBlackDeco)
									parts[nr].dcolour = COLRGB(0, 0, 0);
							}
						}
						else if (!destroy)
						{
							if (rt == PT_BRAY)
							{
								switch (parts[r].tmp)
								{
								case 0:
									// if it hits another BRAY that isn't red
									if (nyy!=0 || nxx!=0)
									{
										parts[r].life = long_bray_life; // makes it last a while
										parts[r].tmp = 1;
										if (!parts[r].ctype) // and colors it if it isn't already
											parts[r].ctype = colored;
									}
								case 2://red bray, stop
								default:
									docontinue = 0;
									break;
								case 1://if it hits one that already was a long life, reset it
									parts[r].life = long_bray_life;
									//docontinue = 1;
									break;
								}
								if (isBlackDeco)
									parts[r].dcolour = COLRGB(0, 0, 0);
							}
							// get color if passed through FILT
							else if (rt==PT_FILT)
							{
								if (parts[r].tmp != 6)
								{
									colored = interactWavelengths(&parts[r], colored);
									if (!colored)
										break;
								}
								isBlackDeco = (parts[r].dcolour==COLRGB(0, 0, 0));
								parts[r].life = 4;
							}
							else if (rt == PT_STOR)
							{
								if (parts[r].tmp)
								{
									// Cause STOR to release
									for (int ry1 = 1; ry1 >= -1; ry1--)
									{
										for (int rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1)
										{
											int np = sim->part_create(-1, x+nxi+nxx+rx1, y+nyi+nyy+ry1, TYP(parts[r].tmp));
											if (np != -1)
											{
												parts[np].temp = parts[r].temp;
												parts[np].life = parts[r].tmp2;
												parts[np].tmp = parts[r].tmp3;
												parts[np].ctype = parts[r].tmp4;
												parts[r].tmp = 0;
												parts[r].life = 10;
												break;
											}
										}
									}
								}
								else
								{
									parts[r].life = 10;
								}
							}
							//this if prevents BRAY from stopping on certain materials
							else if (rt!=PT_INWR && (rt!=PT_SPRK || parts[r].ctype!=PT_INWR) && rt!=PT_ARAY && rt!=PT_WIFI && !(rt==PT_SWCH && parts[r].life>=10))
							{
								if ((nyy!=0 || nxx!=0) && rt != PT_WIRE)
								{
									sim->spark_all_attempt(r, x+nxi+nxx, y+nyi+nyy);
								}
								if (!(nostop && parts[r].type==PT_SPRK && parts[r].ctype >= 0 && parts[r].ctype < PT_NUM && (sim->elements[parts[r].ctype].Properties & PROP_CONDUCTS)))
									docontinue = 0;
								else
									docontinue = 1;
							}
						}
						else if (destroy)
						{
							if (rt == PT_BRAY)
							{
								parts[r].tmp = 2;
								parts[r].life = 1;
								docontinue = 1;
								if (isBlackDeco)
									parts[r].dcolour = COLRGB(0, 0, 0);
							//this if prevents red BRAY from stopping on certain materials
							}
							else if (rt==PT_STOR || rt==PT_INWR || (rt==PT_SPRK && parts[r].ctype==PT_INWR) || rt==PT_ARAY || rt==PT_WIFI || rt==PT_FILT || (rt==PT_SWCH && parts[r].life>=10))
							{
								if (rt == PT_STOR)
								{
									parts[r].tmp = 0;
									parts[r].life = 0;
								}
								else if (rt == PT_FILT)
								{
									isBlackDeco = (parts[r].dcolour==COLRGB(0, 0, 0));
									parts[r].life = 2;
								}
								docontinue = 1;
							}
							else
							{
								docontinue = 0;
							}
						}
					}
				}
				//parts[i].life = 4;
			}
	return 0;
}

void ARAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ARAY";
	elem->Name = "ARAY";
	elem->Colour = COLPACK(0xFFBB00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
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

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Ray Emitter. Rays create points when they collide.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &ARAY_update;
	elem->Graphics = NULL;
	elem->Init = &ARAY_init_element;
}
