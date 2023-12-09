/**
 * Powder Toy - saving and loading functions header
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
#ifndef SAVEOLD_H
#define SAVEOLD_H
#include "BSON.h"
#include "json/json.h"
#include "simulation/ElementNumbers.h"
#include "simulation/SimulationData.h"
#include "graphics/Pixel.h"

//builds a thumb or something? idk
void *build_thumb(int *size, int bzip2);

//calls the correct function to prerender either an OPS or PSV save
pixel *prerender_save(void *save, int size, int *width, int *height);

//converts mod elements from older saves into the new correct id's, since as new elements are added to tpt the id's go up
int fix_type(int type, int version, int modver, int (elementPalette)[PT_NUM] = NULL);

//functions to check saves for invalid elements that shouldn't be saved
int invalid_element(int save_as, int el);
int check_save(int save_as, int orig_x0, int orig_y0, int orig_w, int orig_h, int give_warning);

//converts old format and new tpt++ format wall id's into the correct id's for this version
int change_wall(int wt);
int change_wallpp(int wt);

//Current save prerenderer
pixel *prerender_save_OPS(void *save, int size, int *width, int *height);

//Old save prerenderer
pixel *prerender_save_PSv(void *save, int size, int *width, int *height);

#endif
