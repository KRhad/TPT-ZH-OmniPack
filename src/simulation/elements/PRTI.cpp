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
#include "simulation/elements/PRTI.h"
#include "simulation/elements/SOAP.h"

/*these are the count values of where the particle gets stored, depending on where it came from
   0 1 2
   7 . 3
   6 5 4
   PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
   PRTO does +/-1 to the count, so it doesn't jam as easily
*/
const int portal_rx[8] = {-1, 0, 1, 1, 1, 0,-1,-1};
const int portal_ry[8] = {-1,-1,-1, 0, 1, 1, 1, 0};

int PRTI_update(UPDATE_FUNC_ARGS)
{
	if (!sim->elementData[PT_PRTI])
		return 0;
#ifndef NOMOD
	if (parts[i].type == PT_PPTI && parts[i].tmp2 < 10)
		return 0;
#endif
	int fe = 0;
	PortalChannel *channel = static_cast<PRTI_ElementDataContainer&>(*sim->elementData[PT_PRTI]).GetParticleChannel(sim, i);
	for (int count = 0; count < 8; count++)
	{
		if (channel->particleCount[count] >= channel->storageSize)
			continue;
		int rx = portal_rx[count];
		int ry = portal_ry[count];
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r || TYP(r) == PT_STOR)
				fe = 1;
			if (!r || (!(sim->elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)) && TYP(r)!=PT_SPRK && TYP(r)!=PT_STOR))
			{
				r = photons[y+ry][x+rx];
				if (!r)
					continue;
			}

			if (TYP(r) == PT_STKM || TYP(r) == PT_STKM2 || TYP(r) == PT_FIGH)
				continue;// Handling these is a bit more complicated, and is done in STKM_interact()

			if (TYP(r) == PT_SOAP)
				SOAP_detach(ID(r));

			if (channel->StoreParticle(sim, ID(r), count))
				fe = 1;
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
			if (orbd[r] > 1)
			{
				orbd[r] -= 12;
				if (orbd[r] < 1)
				{
					orbd[r] = RNG::Ref().between(128, 255);
					orbl[r] = RNG::Ref().between(0, 254);
				}
				else
				{
					orbl[r] += 2;
					orbl[r] = orbl[r]%255;
				}
			}
			else
			{
				orbd[r] = RNG::Ref().between(128, 255);
				orbl[r] = RNG::Ref().between(0, 254);
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

int PRTI_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 8;
	*firer = 255;
	*fireg = 0;
	*fireb = 0;
	*pixel_mode |= EFFECT_GRAVIN;
	*pixel_mode |= EFFECT_DBGLINES;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_ADD;
	return 1;
}

void PRTI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PRTI";
	elem->Name = "PRTI";
	elem->Colour = COLPACK(0xEB5917);
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
	elem->HotAir = -0.005f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "传送门入口。粒子进入，温度依赖频道（同WIFI）。";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PRTI_update;
	elem->Graphics = &PRTI_graphics;
	elem->Init = &PRTI_init_element;


	sim->elementData[t].reset(new PRTI_ElementDataContainer);
}
