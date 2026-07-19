#include <cmath> // for pow
#include <cstdlib> // needed in some compilers for std::abs
#include "Brush.h"
#include "common/tpt-minmax.h"

Brush *currentBrush;
bool perfectCircleBrush = false;

void Brush::GenerateBitmap()
{
	if (bitmap)
		delete[] bitmap;
	int sizeX = radius.X + radius.X + 1;
	int sizeY = radius.Y + radius.Y + 1;
	bitmap = new bool[sizeX * sizeY];
	std::fill(&bitmap[0], &bitmap[sizeX * sizeY], false);

	int rx = radius.X, ry = radius.Y;
	int x = rx, y = ry;
	if (rx <= 0) //workaround for rx == 0 crashing (does this still crash?)
	{
		for (int j = y - ry; j <= y + ry; j++)
			bitmap[(j * rx) + x] = true;
	}
	else
	{
		int tempy = y, i, j, jmax;
		// tempy is the smallest y value that is still inside the brush
		// jmax is the largest y value that is still inside the brush (bottom border of brush)

		//For triangle brush, start at the very bottom
		if (shape == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++)
		{
			//loop up until it finds a point not in the brush
			while (tempy >= y - ry && IsInside(i - x, tempy - y))
				tempy = tempy - 1;
			tempy = tempy + 1;

			//If triangle brush, create parts down to the bottom always; if not go down to the bottom border
			if (shape == TRI_BRUSH)
				jmax = y + ry;
			else
				jmax = 2 * y - tempy;
			for (j = tempy; j <= jmax; j++)
			{
				bitmap[(j * sizeX) + i] = true;
				//don't create twice in the vertical center line
				if (i != x)
					bitmap[(j * sizeX) + (2 * x - i)] = true;
			}
		}
	}
}

// May return true for coordinates NOT inside the brush's radius, in the case of perfect circle brush. This happens for points on each axis
bool Brush::IsInside(int x, int y)
{
	switch (shape)
	{
	case CIRCLE_BRUSH:
		// Perfect circle mode does rounding so points just slightly outside the circle according to the formula still count
		if (perfectCircleBrush)
			return (pow(x, 2.0) * pow(radius.Y - 0.5, 2.0) + pow(y, 2.0) * pow(radius.X - 0.5, 2.0) <= pow(radius.X, 2.0) * pow(radius.Y, 2.0));
		// Original TPT brush we all know and love :)
		else
			return (pow(x, 2.0) * pow(radius.Y, 2.0) + pow(y, 2.0) * pow(radius.X, 2.0) <= pow(radius.X, 2.0) * pow(radius.Y, 2.0));
		break;
	case SQUARE_BRUSH:
		return (std::abs(x) <= radius.X && std::abs(y) <= radius.Y);
		break;
	case TRI_BRUSH:
		return ((std::abs((radius.X + 2 * x) * radius.Y + radius.X * y)
				 + std::abs(2 * radius.X * (y - radius.Y)) + std::abs((radius.X - 2 * x) * radius.Y + radius.X * y))
				<= (4 * radius.X * radius.Y));
		break;
	default:
		return 0;
		break;
	}
}

void Brush::SetRadius(Point radius_)
{
	radius = radius_;
	radius.X = std::min(1180, std::max(0, radius.X));
	radius.Y = std::min(1180, std::max(0, radius.Y));
	GenerateBitmap();
}

void Brush::ChangeRadius(Point change)
{
	radius.X += change.X;
	radius.Y += change.Y;
	radius.X = std::min(1180, std::max(0, radius.X));
	radius.Y = std::min(1180, std::max(0, radius.Y));
	GenerateBitmap();
}

void Brush::SetShape(int shape_)
{
	shape = shape_;
	GenerateBitmap();
}
