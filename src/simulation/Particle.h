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

#ifndef Particle_h
#define Particle_h

#include <vector>
#include "StructProperty.h"
#include "graphics/ARGBColour.h"

struct particle
{
	int type;
	int life, ctype;
	float x, y, vx, vy;
	float temp;
	int flags;
	int tmp;
	int tmp2;
	int tmp3;
	int tmp4;
	ARGBColour dcolour;

	/** Returns a list of properties, their type and offset within the structure that can be changed
	 by higher-level processes referring to them by name such as Lua or the property tool **/
	static std::vector<StructProperty> const &GetProperties();
	static std::vector<StructPropertyAlias> const &GetPropertyAliases();
	static StructProperty PropertyByName(const std::string& Name);
	static std::vector<int> const &PossiblyCarriesType();

private:
	static std::vector<StructProperty> properties;
};
using particle = struct particle;

int Particle_GetOffset(std::string key, int * format);

// important: these are indices into the vector returned by Particle::GetProperties, not indices into Particle
constexpr unsigned int FIELD_LIFE  =  1;
constexpr unsigned int FIELD_CTYPE =  2;
constexpr unsigned int FIELD_TMP   =  9;
constexpr unsigned int FIELD_TMP2  = 10;
constexpr unsigned int FIELD_TMP3  = 11;
constexpr unsigned int FIELD_TMP4  = 12;

#endif
