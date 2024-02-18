#ifndef BRUSH_H
#define BRUSH_H

#include "common/Point.h"

enum
{
	CIRCLE_BRUSH,
	SQUARE_BRUSH,
	TRI_BRUSH,
	NUM_DEFAULTBRUSHES
};

//TODO: maybe use bitmaps for actual drawing and support tpt++ custom brushes?
class Brush
{
	Point radius;
	int shape;
	// this is only used for lua sim.brush at the moment, not for actual drawing
	// in the future, actual brushes and custom brushes can be stored in here
	bool * bitmap;

	void GenerateBitmap();

public:
	Brush(Point radius_, int shape_) :
		radius(radius_),
		shape(shape_),
		bitmap(nullptr)
	{
		GenerateBitmap();
	}

	~Brush() { if (bitmap) delete[] bitmap; }

	void SetRadius(Point radius_);
	void ChangeRadius(Point change);
	Point GetRadius() { return radius; }
	void SetShape(int shape_);
	int GetShape() { return shape; }
	bool * GetBitmap() { return bitmap; }
	bool IsInside(int x, int y);
};

extern Brush *currentBrush;
extern bool perfectCircleBrush;

#endif
