/**
 * Powder Toy - simple console (header)
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
#ifndef LEGACY_CONSOLE_H
#define LEGACY_CONSOLE_H

#include <defines.h>
#include <graphics/Pixel.h>
extern int file_script;

class Simulation;
int console_parse_coords(const char *txt, int *x, int *y, char *err);
int console_parse_type(const char *txt, int *element, char *err, Simulation *sim);
int console_parse_wall_type(const char *txt, int *wall);
int console_parse_partref(const char *txt, int *which, char *err);

int process_command_old(Simulation * sim, pixel *vid_buf, const char *console, std::string *console_error);

#endif
