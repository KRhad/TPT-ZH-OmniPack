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
#include "simulation/elements/FIGH.h"
#include "simulation/elements/PRTI.h"
#include "simulation/elements/STKM.h"

/*these are the count values of where the particle gets stored, depending on where it came from
   0 1 2
   7 . 3
   6 5 4
   PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
   PRTO does +/-1 to the count, so it doesn't jam as easily
*/
int PRTO_update(UPDATE_FUNC_ARGS)
{
	if (!sim->elementData[PT_PRTI])
		return 0;
#ifndef NOMOD
	if (parts[i].type == PT_PPTO && parts[i].tmp2 < 10)
		return 0;
#endif
	int fe = 0;
	PortalChannel *channel = static_cast<PRTI_ElementDataContainer&>(*sim->elementData[PT_PRTI]).GetParticleChannel(sim, i);
	for (int count = 0; count < 8; count++)
	{
		int rx = portal_rx[count];
		int ry = portal_ry[count];
		if (rx || ry)
		{
			if (!pmap[y+ry][x+rx])
			{
				fe = 1;
				for (int nnx = 0 ; nnx < PortalChannel::storageSize; nnx++)
				{
					//add -1,0,or 1 to count
					int randomness = (count + RNG::Ref().between(-1, 1) + 4)%8;
					if (!channel->portalp[randomness][nnx].type)
						continue;
					particle *storedPart = &(channel->portalp[randomness][nnx]);
					if (storedPart->type == PT_SPRK)// TODO: make it look better, spark creation
					{
						if (pmap[y+1][x+1])
							sim->spark_all_attempt(ID(pmap[y+1][x+1]), x+1, y+1);
						if (pmap[y][x+1])
							sim->spark_all_attempt(ID(pmap[y][x+1]), x+1, y);
						if (pmap[y-1][x+1])
							sim->spark_all_attempt(ID(pmap[y-1][x+1]), x+1, y-1);
						if (pmap[y+1][x])
							sim->spark_all_attempt(ID(pmap[y+1][x]), x, y+1);
						if (pmap[y-1][x])
							sim->spark_all_attempt(ID(pmap[y-1][x]), x, y-1);
						if (pmap[y+1][x-1])
							sim->spark_all_attempt(ID(pmap[y+1][x-1]), x-1, y+1);
						if (pmap[y][x-1])
							sim->spark_all_attempt(ID(pmap[y][x-1]), x-1, y);
						if (pmap[y-1][x-1])
							sim->spark_all_attempt(ID(pmap[y-1][x-1]), x-1, y-1);
						storedPart->type = 0;
						channel->particleCount[randomness]--;
						break;
					}
					else if (storedPart->type)
					{
						if (storedPart->type == PT_FIGH)
						{
							static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Free((unsigned char)storedPart->tmp);
						}
						else if (storedPart->type == PT_STKM)
							static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1()->spwn = 0;
						else if (storedPart->type == PT_STKM2)
							static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2()->spwn = 0;
						int np = sim->part_create(-1,x+rx,y+ry,storedPart->type);
						if (np < 0)
						{
							if (storedPart->type == PT_FIGH)
							{
								static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).AllocSpecific((unsigned char)storedPart->tmp);
							}
							else if (storedPart->type == PT_STKM)
								static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman1()->spwn = 1;
							else if (storedPart->type == PT_STKM2)
								static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).GetStickman2()->spwn = 1;
							continue;
						}
						if (parts[np].type == PT_FIGH)
						{
							// Release the fighters[] element allocated by part_create, the one reserved when the fighter went into the portal will be used
							static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).Free((unsigned char)parts[np].tmp);
							static_cast<FIGH_ElementDataContainer&>(*sim->elementData[PT_FIGH]).AllocSpecific((unsigned char)storedPart->tmp);
						}
						if (storedPart->vx == 0.0f && storedPart->vy == 0.0f)
						{
							// particles that have passed from PIPE into PRTI have lost their velocity, so use the velocity of the newly created particle if the particle in the portal has no velocity
							float tmp_vx = parts[np].vx;
							float tmp_vy = parts[np].vy;
							parts[np] = *storedPart;
							parts[np].vx = tmp_vx;
							parts[np].vy = tmp_vy;
						}
						else
						{
							parts[np] = *storedPart;
						}
						parts[np].x = (float)(x+rx);
						parts[np].y = (float)(y+ry);
						storedPart->type = 0;
						channel->particleCount[randomness]--;
						break;
					}
				}
			}
		}
	}
	if (fe)
	{
		int orbd[4] = {0, 0, 0, 0};	//Orbital distances
		int orbl[4] = {0, 0, 0, 0};	//Orbital locations
		if (!parts[i].life)
			parts[i].life = RNG::Ref().gen();
		if (!parts[i].ctype)
			parts[i].ctype = RNG::Ref().gen();
		orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
		for (int r = 0; r < 4; r++)
		{
			if (orbd[r] < 254)
			{
				orbd[r] += 16;
				if (orbd[r] > 254)
				{
					orbd[r] = 0;
					orbl[r] = RNG::Ref().between(0, 254);
				}
				else
				{
					orbl[r] += 1;
					orbl[r] = orbl[r]%255;
				}
			}
			else
			{
				orbd[r] = 0;
				orbl[r] =RNG::Ref().between(0, 254);
			}
		}
		orbitalparts_set(&parts[i].life, &parts[i].ctype, orbd, orbl);
	}
	else
	{
		parts[i].life = 0;
		parts[i].ctype = 0;
	}
	return 0;
}

int PRTO_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 8;
	*firer = 0;
	*fireg = 0;
	*fireb = 255;
	*pixel_mode |= EFFECT_GRAVOUT;
	*pixel_mode |= EFFECT_DBGLINES;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_ADD;
	return 1;
}

void PRTO_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PRTO";
	elem->Name = "PRTO";
	elem->Colour = COLPACK(0x0020EB);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.005f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "出口传送门。粒子从这里出来；通道随温度变化（与 WIFI 相同）。";
	elem->DetailedDescription = "描述：和传送门入口(PRTI)配套，物质从这里出来，形成正压力。\n导热率：0\n初始温度：22℃/295.15K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PRTO_update;
	elem->Graphics = &PRTO_graphics;
	elem->Init = &PRTO_init_element;
}
