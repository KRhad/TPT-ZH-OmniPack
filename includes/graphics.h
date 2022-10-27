/**
 * Powder Toy - graphics (header)
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
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string>
#include "defines.h"
#include "graphics/Pixel.h"

class Simulation;

extern pixel sampleColor;

extern unsigned int render_mode;
extern unsigned int display_mode;


extern unsigned char fire_r[YRES/CELL][XRES/CELL];
extern unsigned char fire_g[YRES/CELL][XRES/CELL];
extern unsigned char fire_b[YRES/CELL][XRES/CELL];

extern unsigned int fire_alpha[CELL*3][CELL*3];
extern pixel *pers_bg;

extern char * flm_data;
extern int flm_data_points;
extern pixel flm_data_colours[];
extern float flm_data_pos[];

extern char * plasma_data;
extern int plasma_data_points;
extern pixel plasma_data_colours[];
extern float plasma_data_pos[];

struct gcache_item
{
	int isready;
	int pixel_mode;
	int cola, colr, colg, colb;
	int firea, firer, fireg, fireb;
};
typedef struct gcache_item gcache_item;

extern gcache_item *graphicscache;

void prepare_graphicscache();

void draw_other(pixel *vid, Simulation * sim);

void draw_rgba_image(pixel *vid, unsigned char *data, int x, int y, float a);

void render_gravlensing(pixel *src, pixel * dst);

void drawblob(pixel *vid, int x, int y, unsigned char cr, unsigned char cg, unsigned char cb);

void draw_tool_button(pixel *vid_buf, int x, int y, pixel color, std::string name);
int draw_tool_xy(pixel *vid_buf, int x, int y, Tool* currents);

int DrawMenus(pixel *vid_buf, int hover, int mouseY);

int DrawMenusTouch(pixel *vid_buf, int b, int bq, int mx, int my);

void drawpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

int addchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a);

int drawchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a);

int drawtext(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a, bool noColor = false);

int drawhighlight(pixel *vid, int x, int y, const char *s);

int drawtext_outline(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a, int outr, int outg, int outb, int outa);

int drawtextwrap(pixel *vid, int x, int y, int w, int h, const char *s, int r, int g, int b, int a);

int drawhighlightwrap(pixel *vid, int x, int y, int w, int h, const char *s, int highlightstart, int highlightlength);

void drawrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a);

void fillrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a);

void drawcircle(pixel *vid, int x, int y, int rx, int ry, int r, int g, int b, int a);

void fillcircle(pixel *vid, int x, int y, int rx, int ry, int r, int g, int b, int a);

void clearrect(pixel *vid, int x, int y, int w, int h);

void drawdots(pixel *vid, int x, int y, int h, int r, int g, int b, int a);

int charwidth(unsigned char c);

int textwidth(const char *s);

int drawtextmax(pixel *vid, int x, int y, int w, const char *s, int r, int g, int b, int a);

int textnwidth(char *s, int n);

void textnpos(char *s, int n, int w, int *cx, int *cy);

int textwidthx(char *s, int w);

void textsize(char * s, int *width, int *height);

int textposxy(char *s, int width, int w, int h);

int textwrapheight(char *s, int width);

void blendpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

void draw_icon(pixel *vid_buf, int x, int y, char ch, int flag);

pixel HeatToColor(float temp);

void draw_air(pixel *vid, Simulation * sim);

void draw_grav_zones(pixel *vid);

void draw_grav(pixel *vid);

void draw_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int screenwidth);

void addpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

void xor_pixel(int x, int y, pixel *vid);

void xor_line(int x1, int y1, int x2, int y2, pixel *vid);

void xor_rect(pixel *vid, int x, int y, int w, int h);

void blend_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int a);

struct Point;
void render_parts(pixel *vid, Simulation * sim, Point mousePos);

void render_before(pixel *part_vbuf, Simulation * sim);

void render_after(pixel *part_vbuf, pixel *vid_buf, Simulation * sim, Point mousePos);

void draw_parts(pixel *vid);

void draw_walls(pixel *vid, Simulation * sim);

void draw_find(Simulation * sim);

void render_signs(pixel *vid_buf, Simulation * sim);

void render_fire(pixel *dst);

void prepare_alpha(float intensity);

void draw_image(pixel *vid, pixel *img, int x, int y, int w, int h, int a);

void dim_copy(pixel *dst, pixel *src);

void dim_copy_pers(pixel *dst, pixel *src);

void render_zoom(pixel *img);

int render_thumb(void *thumb, int size, int bzip2, pixel *vid_buf, int px, int py, int scl);

class Brush;
void render_cursor(pixel *vid, int x, int y, Tool* t, Brush* brush);

int draw_debug_info(pixel* vid, Simulation * sim, int lx, int ly, int cx, int cy, int line_x, int line_y);

#endif

#ifdef INCLUDE_SHADERS
extern const char * fireFragment;
extern const char * fireVertex;
extern const char * lensFragment;
extern const char * lensVertex;
extern const char * airVFragment;
extern const char * airVVertex;
extern const char * airPFragment;
extern const char * airPVertex;
extern const char * airCFragment;
extern const char * airCVertex;
#endif
