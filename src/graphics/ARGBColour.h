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

/* An ARGB colour value (independent of the pixel format, unlike PIXPACK 
 * and other macros that were previously used)
 * 
 * This is used for the particle default colour so that the simulation
 * code doesn't depend on the pixel format. It is also used for
 * dcolour and all the tool stuff.
 */

#ifndef ARGBColour_h
#define ARGBColour_h

#include "common/tpt-stdint.h"

using ARGBColour = uint32_t;

#define COLPACK(x) (0xFF000000|(x))
#define COLARGB(a,r,g,b) (((a)<<24)|((r)<<16)|((g)<<8)|(b))
#define COLRGB(r,g,b) (0xFF000000|((r)<<16)|((g)<<8)|(b))
#define COLA(x) (((x)>>24)&0xFF)
#define COLR(x) (((x)>>16)&0xFF)
#define COLG(x) (((x)>>8)&0xFF)
#define COLB(x) ((x)&0xFF)

/** Modifies the alpha of this color value */
constexpr int COLMODALPHA(ARGBColour color, unsigned int alpha)
{
	return ((alpha) << 24 | (color & 0x00FFFFFF));
}

/** multiplies rgb components by a certain value */
constexpr int COLMULT(ARGBColour color, float mult)
{
	return COLARGB(COLA(color), static_cast<int>(COLR(color) * (mult)), static_cast<int>(COLG(color) * (mult)), static_cast<int>(COLB(color) * (mult)));
}

/** adds a certain amount to each rgb component */
constexpr int COLADD(ARGBColour color, unsigned int add)
{
	return COLARGB(COLA(color), COLR(color) + (add), COLG(color) + (add), COLB(color) + (add));
}

#endif
