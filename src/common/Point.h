/**
 * Powder Toy - Point (header)
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
#ifndef POINT_H
#define POINT_H

//Note: OS X defines a struct Point, any file that includes both this and a common mac header will error
struct Point
{
	int X = 0;
	int Y = 0;

	Point() = default;
	Point(int pointX, int pointY):
		X(pointX),
		Y(pointY)
	{ }

	void Clamp(Point min, Point max)
	{
		if (X < min.X)
			X = min.X;
		else if (X > max.X)
			X = max.X;

		if (Y < min.Y)
			Y = min.Y;
		else if (Y > max.Y)
			Y = max.Y;
	}

	bool IsInside(Point topLeft, Point bottomRight)
	{
		return X >= topLeft.X && X < bottomRight.X && Y >= topLeft.Y && Y < bottomRight.Y;
	}

	//some basic operator overloads
	inline bool operator == (const Point& other) const
	{
		return (X == other.X && Y == other.Y);
	}

	inline bool operator != (const Point& other) const
	{
		return (X != other.X || Y != other.Y);
	}

	inline Point operator + (const Point& other)
	{
		return Point(X+other.X, Y+other.Y);
	}

	inline Point operator - (const Point& other)
	{
		return Point(X-other.X, Y-other.Y);
	}

	inline Point operator * (const int other)
	{
		return Point(X*other, Y*other);
	}

	inline Point operator / (const int other)
	{
		return Point(X/other, Y/other);
	}

	inline void operator += (const Point& other)
	{
		X += other.X;
		Y += other.Y;
	}

	inline void operator -= (const Point& other)
	{
		X -= other.X;
		Y -= other.Y;
	}
};
using Point = struct Point;

#endif
