/**
 * Powder Toy - particle simulation (header)
 *
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
#ifndef POWDER_H
#define POWDER_H

#include "defines.h"
#include "misc.h"

#define BRUSH_REPLACEMODE 0x1
#define BRUSH_SPECIFIC_DELETE 0x2

#include "simulation/ElementNumbers.h"
#include "simulation/SimulationData.h"


class Simulation;

#include "simulation/Particle.h"

void TRON_init_graphics();

void PPIP_flood_trigger(Simulation* sim, int x, int y, int sparkedBy);

#define CHANNELS ((int)(MAX_TEMP-73)/100+2)
extern const particle emptyparticle;

extern particle *parts;

extern unsigned char bmap[YRES/CELL][XRES/CELL];
extern unsigned char emap[YRES/CELL][XRES/CELL];

extern unsigned pmap[YRES][XRES];
extern int pmap_count[YRES][XRES];

extern unsigned photons[YRES][XRES];

int get_wavelength_bin(int *wm);

void part_change_type(int i, int x, int y, int t);

int get_brush_flags();

int is_wire(int x, int y);

int is_wire_off(int x, int y);

void set_emap(int x, int y);

int parts_avg(int ci, int ni, int t);

int nearest_part(int ci, int t, int max_d);

int INST_flood_spark(Simulation *sim, int x, int y);

void orbitalparts_get(int block1, int block2, int resblock1[], int resblock2[]);
void orbitalparts_set(int *block1, int *block2, int resblock1[], int resblock2[]);

void draw_bframe();
void erase_bframe();

#endif
