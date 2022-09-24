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
#include "simulation/elements/PPIP.h"
#include "simulation/elements/PRTI.h"
#include "graphics.h"

#define PFLAG_NORMALSPEED 0x00010000
#define PFLAG_REVERSE 0x00020000

// 0x000000FF element
// 0x00000100 is single pixel pipe
// 0x00000200 will transfer like a single pixel pipe when in forward mode
// 0x00001C00 forward single pixel pipe direction
// 0x00002000 will transfer like a single pixel pipe when in reverse mode
// 0x0001C000 reverse single pixel pipe direction
// 0x000E0000 PIPE color data stored here

#define PFLAG_NORMALSPEED            0x00010000
#define PFLAG_INITIALIZING           0x00020000 // colors haven't been set yet
#define PFLAG_COLOR_RED              0x00040000
#define PFLAG_COLOR_GREEN            0x00080000
#define PFLAG_COLOR_BLUE             0x000C0000
#define PFLAG_COLORS                 0x000C0000

#define PPIP_TMPFLAG_REVERSED        0x01000000
#define PPIP_TMPFLAG_PAUSED          0x02000000
#define PPIP_TMPFLAG_TRIGGER_REVERSE 0x04000000
#define PPIP_TMPFLAG_TRIGGER_OFF     0x08000000
#define PPIP_TMPFLAG_TRIGGER_ON      0x10000000
#define PPIP_TMPFLAG_TRIGGERS        0x1C000000

signed char pos_1_rx[] = { -1,-1,-1, 0, 0, 1, 1, 1 };
signed char pos_1_ry[] = { -1, 0, 1,-1, 1,-1, 0, 1 };

static void transformPatch(particle &part, const int (&patch)[8])
{
	if (part.tmp & 0x00000200) part.tmp = (part.tmp & 0xFFFFE3FF) | (patch[(part.tmp & 0x00001C00) >> 10] << 10);
	if (part.tmp & 0x00002000) part.tmp = (part.tmp & 0xFFFE3FFF) | (patch[(part.tmp & 0x0001C000) >> 14] << 14);
}

void PIPE_patchR(particle &part)
{
	// 035 -> 210
	// 1 6 -> 4 3
	// 247 -> 765
	const int patchR[] = { 2, 4, 7, 1, 6, 0, 3, 5 };
	transformPatch(part, patchR);
}

void PIPE_patchH(particle &part)
{
	// 035 -> 530
	// 1 6 -> 6 1
	// 247 -> 742
	const int patchH[] = { 5, 6, 7, 3, 4, 0, 1, 2 };
	transformPatch(part, patchH);
}

void PIPE_patchV(particle &part)
{
	// 035 -> 247
	// 1 6 -> 1 6
	// 247 -> 035
	const int patchV[] = { 2, 1, 0, 4, 3, 7, 6, 5 };
	transformPatch(part, patchV);
}

unsigned int prevColor(unsigned int flags)
{
	unsigned int color = flags & PFLAG_COLORS;
	if (color == PFLAG_COLOR_RED)
		return PFLAG_COLOR_GREEN;
	else if (color == PFLAG_COLOR_GREEN)
		return PFLAG_COLOR_BLUE;
	else if (color == PFLAG_COLOR_BLUE)
		return PFLAG_COLOR_RED;
	return PFLAG_COLOR_RED;
}

unsigned int nextColor(unsigned int flags)
{
	unsigned int color = flags & PFLAG_COLORS;
	if (color == PFLAG_COLOR_RED)
		return PFLAG_COLOR_BLUE;
	else if (color == PFLAG_COLOR_BLUE)
		color = PFLAG_COLOR_GREEN;
	else if (color == PFLAG_COLOR_GREEN)
		return PFLAG_COLOR_RED;
	return PFLAG_COLOR_GREEN;
}

void PPIP_flood_trigger(Simulation* sim, int x, int y, int sparkedBy)
{
	int coord_stack_limit = XRES*YRES;
	unsigned short (*coord_stack)[2];
	int coord_stack_size = 0;
	int x1, x2;

	// Separate flags for on and off in case PPIP is sparked by PSCN and NSCN on the same frame
	// - then PSCN can override NSCN and behaviour is not dependent on particle order
	int prop = 0;
	if (sparkedBy == PT_PSCN)
		prop = PPIP_TMPFLAG_TRIGGER_ON << 3;
	else if (sparkedBy == PT_NSCN)
		prop = PPIP_TMPFLAG_TRIGGER_OFF << 3;
	else if (sparkedBy == PT_INST)
		prop = PPIP_TMPFLAG_TRIGGER_REVERSE << 3;

	if (prop == 0 || TYP(pmap[y][x]) != PT_PPIP || (parts[ID(pmap[y][x])].tmp & prop))
		return;

	coord_stack = (unsigned short(*)[2])malloc(sizeof(unsigned short)*2*coord_stack_limit);
	coord_stack[coord_stack_size][0] = x;
	coord_stack[coord_stack_size][1] = y;
	coord_stack_size++;

	do
	{
		coord_stack_size--;
		x = coord_stack[coord_stack_size][0];
		y = coord_stack[coord_stack_size][1];
		x1 = x2 = x;
		// go left as far as possible
		while (x1 >=CELL)
		{
			if (TYP(pmap[y][x1-1]) != PT_PPIP)
			{
				break;
			}
			x1--;
		}
		// go right as far as possible
		while (x2 < XRES - CELL)
		{
			if (TYP(pmap[y][x2+1]) != PT_PPIP)
			{
				break;
			}
			x2++;
		}
		// fill span
		for (x = x1; x <= x2; x++)
		{
			if (!(parts[ID(pmap[y][x])].tmp & prop))
				static_cast<PPIP_ElementDataContainer&>(*sim->elementData[PT_PPIP]).ppip_changed = 1;
			parts[ID(pmap[y][x])].tmp |= prop;
		}

		// add adjacent pixels to stack
		// +-1 to x limits to include diagonally adjacent pixels
		// Don't need to check x bounds here, because already limited to [CELL, XRES-CELL]
		if (y >= CELL+1)
			for (x = x1-1; x <= x2 + 1; x++)
				if (TYP(pmap[y-1][x])==PT_PPIP && !(parts[ID(pmap[y-1][x])].tmp & prop))
				{
					coord_stack[coord_stack_size][0] = x;
					coord_stack[coord_stack_size][1] = y-1;
					coord_stack_size++;
					if (coord_stack_size >= coord_stack_limit)
					{
						free(coord_stack);
						return;
					}
				}
		if (y < YRES - CELL - 1)
			for (x = x1 - 1; x <= x2 + 1; x++)
				if (TYP(pmap[y+1][x]) == PT_PPIP && !(parts[ID(pmap[y+1][x])].tmp & prop))
				{
					coord_stack[coord_stack_size][0] = x;
					coord_stack[coord_stack_size][1] = y+1;
					coord_stack_size++;
					if (coord_stack_size >= coord_stack_limit)
					{
						free(coord_stack);
						return;
					}
				}
	} while (coord_stack_size > 0);
	free(coord_stack);
}

void PIPE_transfer_pipe_to_part(Simulation *sim, particle *pipe, particle *part, bool STOR)
{
	// STOR also calls this function to move particles from STOR to PRTI
	// PIPE was changed, so now PIPE and STOR don't use the same particle storage format
	if (STOR)
	{
		part->type = TYP(pipe->tmp);
		pipe->tmp = 0;
	}
	else
	{
		part->type = TYP(pipe->ctype);
		pipe->ctype = 0;
	}
	part->temp = pipe->temp;
	part->life = pipe->tmp2;
	part->tmp = pipe->tmp3;
	part->ctype = pipe->tmp4;

	if (!(sim->elements[part->type].Properties & TYPE_ENERGY))
	{
		part->vx = 0.0f;
		part->vy = 0.0f;
	}
	part->tmp2 = 0;
	part->flags = 0;
	part->dcolour = COLARGB(0, 0, 0, 0);
}

void PIPE_transfer_part_to_pipe(particle *part, particle *pipe)
{
	pipe->ctype = part->type;
	pipe->temp = part->temp;
	pipe->tmp2 = part->life;
	pipe->tmp3 = part->tmp;
	pipe->tmp4 = part->ctype;
}

void PIPE_transfer_pipe_to_pipe(particle *src, particle *dest, bool STOR=false)
{
	// STOR to PIPE
	if (STOR)
	{
		dest->ctype = src->tmp;
		src->tmp = 0;
	}
	else
	{
		dest->ctype = src->ctype;
		src->ctype = 0;
	}
	dest->temp = src->temp;
	dest->tmp2 = src->tmp2;
	dest->tmp3 = src->tmp3;
	dest->tmp4 = src->tmp4;
}

void pushParticle(Simulation *sim, int i, int count, int original)
{
	// Don't push if there is nothing there, max speed of 2 per frame
	if (!TYP(parts[i].ctype) || count >= 2)
		return;
	unsigned int notctype = nextColor(sim->parts[i].tmp);
	int x = (int)(parts[i].x + 0.5f);
	int y = (int)(parts[i].y + 0.5f);
	if (!(parts[i].tmp & 0x200))
	{ 
		//normal random push
		int rndstore = RNG::Ref().gen();
		// RAND_MAX is at least 32767 on all platforms i.e. pow(8,5)-1
		// so can go 5 cycles without regenerating rndstore
		// Try to push 3 times
		for (int q = 0; q < 3; q++)
		{
			int rnd = rndstore&7;
			rndstore = rndstore>>3;
			int rx = pos_1_rx[rnd];
			int ry = pos_1_ry[rnd];
			if (BOUNDS_CHECK)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				else if ((TYP(r)==PT_PIPE || TYP(r) == PT_PPIP) && (sim->parts[ID(r)].tmp & PFLAG_COLORS) != notctype && !TYP(sim->parts[ID(r)].ctype))
				{
					PIPE_transfer_pipe_to_pipe(parts + i, parts + (ID(r)));
					// Skip particle push, normalizes speed
					if (ID(r) > original)
						parts[ID(r)].flags |= PFLAG_NORMALSPEED;
					count++;
					pushParticle(sim, ID(r), count, original);
				}
				// Pass particles into PRTI for a pipe speed increase
				else if (TYP(r) == PT_PRTI)
				{
					PortalChannel *channel = static_cast<PRTI_ElementDataContainer&>(*sim->elementData[PT_PRTI]).GetParticleChannel(sim, ID(r));
					int slot = PRTI_ElementDataContainer::GetSlot(-rx, -ry);
					particle *storePart = channel->AllocParticle(slot);
					if (storePart)
					{
						PIPE_transfer_pipe_to_part(sim, parts+i, storePart);
						count++;
						break;
					}
				}
			}
		}
	}
	// Predefined 1 pixel thick pipe movement
	else
	{
		int coords = 7 - ((parts[i].tmp >> 10) & 7);
		int r = pmap[y+ pos_1_ry[coords]][x+ pos_1_rx[coords]];
		if ((TYP(r) == PT_PIPE || TYP(r) == PT_PPIP) && (sim->parts[ID(r)].tmp & PFLAG_COLORS) != notctype && !TYP(sim->parts[ID(r)].ctype))
		{
			PIPE_transfer_pipe_to_pipe(parts + i, parts + (ID(r)));
			// Skip particle push, normalizes speed
			if (ID(r) > original)
				parts[ID(r)].flags |= PFLAG_NORMALSPEED;
			count++;
			pushParticle(sim, ID(r),count,original);
		}
		// Pass particles into PRTI for a pipe speed increase
		else if (TYP(r) == PT_PRTI)
		{
			PortalChannel *channel = static_cast<PRTI_ElementDataContainer&>(*sim->elementData[PT_PRTI]).GetParticleChannel(sim, ID(r));
			int slot = PRTI_ElementDataContainer::GetSlot(-pos_1_rx[coords], -pos_1_ry[coords]);
			particle *storePart = channel->AllocParticle(slot);
			if (storePart)
			{
				PIPE_transfer_pipe_to_part(sim, parts+i, storePart);
				count++;
			}
		}
		// Move particles out of pipe automatically, much faster at ends
		else if (TYP(r) == PT_NONE)
		{
			int rx = pos_1_rx[coords];
			int ry = pos_1_ry[coords];
			int np = sim->part_create(-1, x + rx, y + ry, TYP(parts[i].ctype));
			if (np != -1)
			{
				PIPE_transfer_pipe_to_part(sim, parts+i, parts+np);
			}
		}
	}
	return;
}

void detach(int i);

int PIPE_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].ctype && !sim->elements[TYP(parts[i].ctype)].Enabled)
		parts[i].ctype = 0;
	if (parts[i].tmp & PPIP_TMPFLAG_TRIGGERS)
	{
		int pause_changed = 0;
		// TRIGGER_ON overrides TRIGGER_OFF
		if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_ON)
		{
			if (parts[i].tmp & PPIP_TMPFLAG_PAUSED)
				pause_changed = 1;
			parts[i].tmp &= ~PPIP_TMPFLAG_PAUSED;
		}
		else if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_OFF)
		{
			if (!(parts[i].tmp & PPIP_TMPFLAG_PAUSED))
				pause_changed = 1;
			parts[i].tmp |= PPIP_TMPFLAG_PAUSED;
		}
		if (pause_changed)
		{
			for (int rx = -2; rx <= 2; rx++)
				for (int ry = -2; ry <= 2; ry++)
				{
					if (BOUNDS_CHECK && (rx || ry))
					{
						int r = pmap[y+ry][x+rx];
						if (TYP(r) == PT_BRCK)
						{
							if (parts[i].tmp & PPIP_TMPFLAG_PAUSED)
								parts[ID(r)].tmp = 0;
							// Make surrounding BRCK glow
							else
								parts[ID(r)].tmp = 1;
						}
					}
				}
		}

		if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_REVERSE)
		{
			parts[i].tmp ^= PPIP_TMPFLAG_REVERSED;
			// Switch colors so it goes in reverse
			if ((parts[i].tmp&PFLAG_COLORS) != PFLAG_COLOR_GREEN)
				parts[i].tmp ^= PFLAG_COLOR_GREEN;
			// Switch one pixel pipe direction
			if (parts[i].tmp & 0x100)
			{
				int coords = (parts[i].tmp >> 13) & 0xF;
				int coords2 = (parts[i].tmp >> 9) & 0xF;
				parts[i].tmp &= ~0x1FE00;
				parts[i].tmp |= coords << 9;
				parts[i].tmp |= coords2 << 13;
			}
		}

		parts[i].tmp &= ~PPIP_TMPFLAG_TRIGGERS;
	}

	if ((parts[i].tmp & PFLAG_COLORS) && !(parts[i].tmp & PPIP_TMPFLAG_PAUSED))
	{
		if (parts[i].life == 3)
		{
			int lastneighbor = -1;
			int neighborcount = 0;
			int count = 0;
			// Make automatic pipe pattern
			for (int rx = -1; rx <= 1; rx++)
				for (int ry = -1; ry <= 1; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						count++;
						int r = pmap[y+ry][x+rx];
						if (!r)
							continue;
						if (TYP(r) != PT_PIPE && TYP(r) != PT_PPIP)
							continue;
						unsigned int next = nextColor(parts[i].tmp);
						unsigned int prev = prevColor(parts[i].tmp);
						if (parts[ID(r)].tmp & PFLAG_INITIALIZING)
						{
							parts[ID(r)].tmp |= next;
							parts[ID(r)].tmp &= ~PFLAG_INITIALIZING;
							parts[ID(r)].life = 6;
							// Is a single pixel pipe
							if (parts[i].tmp & 0x100)
							{
								parts[ID(r)].tmp |= 0x200; // Will transfer to a single pixel pipe
								parts[ID(r)].tmp |= (count - 1) << 10;// Coords of where it came from
								parts[i].tmp |= (8 - count) << 14;
								parts[i].tmp |= 0x2000;
							}
							neighborcount ++;
							lastneighbor = ID(r);
						}
						else if ((parts[ID(r)].tmp & PFLAG_COLORS) != prev)
						{
							neighborcount++;
							lastneighbor = ID(r);
						}
					}
			if (neighborcount == 1)
				parts[lastneighbor].tmp |= 0x100;
		}
		else
		{
			// Skip particle push to prevent particle number being higher causing speed up
			if (parts[i].flags & PFLAG_NORMALSPEED)
			{
				parts[i].flags &= ~PFLAG_NORMALSPEED;
			}
			else
			{
				pushParticle(sim, i, 0, i);
			}

			// There is something besides PIPE around current particle
			if (nt)
			{
				int rndstore = RNG::Ref().gen();
				int rnd = rndstore&7;
				rndstore = rndstore>>3;
				int rx = pos_1_rx[rnd];
				int ry = pos_1_ry[rnd];
				if (BOUNDS_CHECK)
				{
					int r = pmap[y+ry][x+rx];
					if(!r)
						r = photons[y+ry][x+rx];
					// Creating at end
					if (surround_space && !r && TYP(parts[i].ctype))
					{
						int np = sim->part_create(-1, x + rx, y + ry, TYP(parts[i].ctype));
						if (np != -1)
						{
							PIPE_transfer_pipe_to_part(sim, parts + i, parts + np);
						}
					}
					// Try eating particle at entrance
					else if (!TYP(parts[i].ctype) && (sim->elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)))
					{
						if (TYP(r) == PT_SOAP)
							detach(ID(r));
						PIPE_transfer_part_to_pipe(parts+(ID(r)), parts + i);
						sim->part_kill(ID(r));
					}
					else if (!TYP(parts[i].ctype) && TYP(r) == PT_STOR && sim->IsElement(parts[ID(r)].tmp) &&
					         (sim->elements[parts[ID(r)].tmp].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)))
					{
						// STOR stores properties in the same places as PIPE does (mostly)
						PIPE_transfer_pipe_to_pipe(parts+(ID(r)), parts + i, true);
					}
				}
			}
		}
	}
	else if (!(parts[i].tmp & (PFLAG_COLORS|PFLAG_INITIALIZING)) && parts[i].life <= 10)
	{
		// Make a border
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
			{
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
					{
						// BRCK border
						int index = sim->part_create(-1, x + rx, y + ry, PT_BRCK);
						if (parts[i].type == PT_PPIP && index != -1)
							parts[index].tmp = 1;
					}
				}
			}
		if (parts[i].life <= 1)
			parts[i].tmp |= PFLAG_INITIALIZING;
	}
	// Wait for empty space before starting to generate automatic pipe pattern
	else if (parts[i].tmp & PFLAG_INITIALIZING)
	{
		if (!parts[i].life)
		{
			for (int rx = -1; rx <= 1; rx++)
				for (int ry = -1; ry <= 1; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						if (!pmap[y+ry][x+rx] && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_ALLOWAIR && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_WALL &&
						        bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_WALLELEC && (bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_EWALL || emap[(y+ry)/CELL][(x+rx)/CELL]))
							parts[i].life = 50;
					}
		}
		// Check for beginning of pipe single pixel
		else if (parts[i].life == 5)
		{
			int issingle = 1;
			for (int rx = -1; rx <= 1; rx++)
				for (int ry = -1; ry <= 1; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						int r = pmap[y+ry][x+rx];
						if ((TYP(r) == PT_PIPE || TYP(r) == PT_PPIP) && parts[i].life)
							issingle = 0;
					}
			if (issingle)
				parts[i].tmp |= 0x100;
		}
		else if (parts[i].life == 2)
		{
			parts[i].tmp |= PFLAG_COLOR_RED;
			parts[i].tmp &= ~PFLAG_INITIALIZING;
			parts[i].life = 6;
		}
	}
	return 0;
}

int PIPE_graphics(GRAPHICS_FUNC_ARGS)
{
	int t = TYP(cpart->ctype);
	if (t > 0 && t < PT_NUM && sim->elements[t].Enabled)
	{
		if (t == PT_STKM || t == PT_STKM2 || t == PT_FIGH)
			return 0;
		if (graphicscache[t].isready)
		{
			*pixel_mode = graphicscache[t].pixel_mode;
			*cola = graphicscache[t].cola;
			*colr = graphicscache[t].colr;
			*colg = graphicscache[t].colg;
			*colb = graphicscache[t].colb;
			*firea = graphicscache[t].firea;
			*firer = graphicscache[t].firer;
			*fireg = graphicscache[t].fireg;
			*fireb = graphicscache[t].fireb;
		}
		else
		{
			// Temp particle used for graphics.
			particle tpart = *cpart;

			// Emulate the graphics of stored particle.
			memset(cpart, 0, sizeof(particle));
			cpart->type = t;
			cpart->temp = tpart.temp;
			cpart->life = tpart.tmp2;
			cpart->tmp = tpart.tmp3;
			cpart->ctype = tpart.tmp4;

			*colr = PIXR(sim->elements[t].Colour);
			*colg = PIXG(sim->elements[t].Colour);
			*colb = PIXB(sim->elements[t].Colour);
			if (sim->elements[t].Graphics)
			{
				(*(sim->elements[t].Graphics))(sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
			}
			else
			{
				graphics_DEFAULT(sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
			}

			// Restore original particle data.
			*cpart = tpart;
		}
	}
	else
	{
		switch (cpart->tmp & PFLAG_COLORS)
		{
		case PFLAG_COLOR_RED:
			*colr = 50;
			*colg = 1;
			*colb = 1;
			break;
		case PFLAG_COLOR_GREEN:
			*colr = 1;
			*colg = 50;
			*colb = 1;
			break;
		case PFLAG_COLOR_BLUE:
			*colr = 1;
			*colg = 1;
			*colb = 50;
			break;
		default:
			break;
		}
	}
	return 0;
}

void PIPE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PIPE";
	elem->Name = "PIPE";
	elem->Colour = COLPACK(0x444444);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "PIPE, moves particles around. Once the BRCK generates, erase some for the exit. Then the PIPE generates and is usable.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 10.0f;
	elem->HighPressureTransitionElement = PT_BRMT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 60;

	elem->Update = &PIPE_update;
	elem->Graphics = &PIPE_graphics;
	elem->Init = &PIPE_init_element;
}
