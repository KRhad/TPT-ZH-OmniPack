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

#ifndef TPTVECTOR_H
#define TPTVECTOR_H

namespace Matrix
{
	// a b
	// c d
	struct matrix2d {
		float a,b,c,d;
	};
	typedef struct matrix2d matrix2d;
	
	// column vector
	struct vector2d {
		float x,y;
	};
	typedef struct vector2d vector2d;
	
	matrix2d m2d_multiply_m2d(matrix2d m1, matrix2d m2);
	vector2d m2d_multiply_v2d(matrix2d m, vector2d v);
	matrix2d m2d_multiply_float(matrix2d m, float s);
	vector2d v2d_multiply_float(vector2d v, float s);
	
	vector2d v2d_add(vector2d v1, vector2d v2);
	vector2d v2d_sub(vector2d v1, vector2d v2);
	
	matrix2d m2d_new(float me0, float me1, float me2, float me3);
	vector2d v2d_new(float x, float y);
	
	extern vector2d v2d_zero;
	extern matrix2d m2d_identity;
	extern matrix2d m2d_mirror_x;
	extern matrix2d m2d_mirror_y;
	extern matrix2d m2d_ccw;
}
#endif // TPTVECTOR_H
