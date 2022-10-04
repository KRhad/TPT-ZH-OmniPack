#include "DirectionSelector.h"
#include "graphics/VideoBuffer.h"

DirectionSelector::DirectionSelector(Point position, float scale, int radius, int handleRadius, int snapPointRadius, int snapPointEffectRadius):
	Component(position, Point(radius * 5 / 2, radius * 5 / 2)),
	scale(scale),
	radius(radius),
	handleRadius(handleRadius),
	useSnapPoints(false),
	snapPointRadius(snapPointRadius),
	snapPointEffectRadius(snapPointEffectRadius),
	autoReturn(false),
	backgroundColor(COLARGB(255, 0, 0, 0)),
	updateCallback(nullptr),
	changeCallback(nullptr),
	mouseDown(false),
	mouseHover(false),
	altDown(false),
	value({ { 0, 0}, 0, 0 })
{

}

void DirectionSelector::CheckHovering(int x, int y)
{
	mouseHover = std::hypot((value.offset.X + radius) - x, (value.offset.Y + radius) - y) < handleRadius;
}

DirectionSelector::Value DirectionSelector::GravityValueToValue(float x, float y)
{
	return { { int(x / scale), int(y / scale) }, x, y };
}

DirectionSelector::Value DirectionSelector::PositionToValue(Point position)
{
	auto length = std::hypot(float(position.X), float(position.Y));
	if (length > radius)
	{
		position.X = int(position.X / length * radius);
		position.Y = int(position.Y / length * radius);
	}
	return { position, position.X * scale, position.Y * scale };
}

void DirectionSelector::SetSnapPoints(int newSnapPointEffectRadius, int points, float maxMagnitude)
{
	snapPointEffectRadius = newSnapPointEffectRadius;
	snapPoints.clear();
	snapPoints.push_back(GravityValueToValue(0, 0));
	for (int i = 1; i < points; i++)
	{
		auto dist = i / float(points - 1) * maxMagnitude;
		snapPoints.push_back(GravityValueToValue( dist,     0));
		snapPoints.push_back(GravityValueToValue(    0,  dist));
		snapPoints.push_back(GravityValueToValue(-dist,     0));
		snapPoints.push_back(GravityValueToValue(    0, -dist));
	}
	useSnapPoints = true;
}

void DirectionSelector::ClearSnapPoints()
{
	useSnapPoints = false;
	snapPoints.clear();
}

float DirectionSelector::GetXValue()
{
	return value.xValue;
}

float DirectionSelector::GetYValue()
{
	return value.yValue;
}

void DirectionSelector::SetPositionAbs(Point position)
{
	SetPosition(position - Point{ (int)(radius + handleRadius), (int)(radius + handleRadius) });
}

void DirectionSelector::SetPosition(Point position)
{
	value = PositionToValue(position);

	if (useSnapPoints && !altDown)
	{
		for (auto &point : snapPoints)
		{
			if (std::hypot(point.offset.X - position.X, point.offset.Y - position.Y) <= snapPointEffectRadius)
			{
				value = point;
			}
		}
	}

	if (updateCallback)
	{
		updateCallback(GetXValue(), GetYValue());
	}
}

void DirectionSelector::SetValues(float x, float y)
{
	value.xValue = x;
	value.yValue = y;
	SetPosition(GravityValueToValue(x, y).offset);
}

void DirectionSelector::OnDraw(gfx::VideoBuffer* vid)
{
	auto handleTrackRadius = radius + handleRadius;
	Point center = Point(position.X + handleTrackRadius, position.Y + handleTrackRadius);

	vid->FillCircle(center.X, center.Y, handleTrackRadius, handleTrackRadius, COLR(backgroundColor), COLG(backgroundColor), COLB(backgroundColor), COLA(backgroundColor));
	vid->DrawCircle(center.X, center.Y, handleTrackRadius, handleTrackRadius, COLR(color), COLG(color), COLB(color), COLA(color));

	for (auto &point : snapPoints)
	{
		vid->FillRect(
			(center.X + point.offset.X) - snapPointRadius,
			(center.Y + point.offset.Y) - snapPointRadius,
			snapPointRadius * 2 + 1, snapPointRadius * 2 + 1,
			COLR(color) / 4, COLG(color) / 4, COLB(color) / 4, altDown ? (int)(COLA(color) / 4) : COLA(color) / 2
		);
	}

	vid->FillCircle(center.X + value.offset.X, center.Y + value.offset.Y, radius / 4, radius / 4, COLR(color) / 4, COLG(color) / 4, COLB(color) / 4, mouseHover ? std::min((int)(COLA(color) * .75), 255) : COLA(color) / 2);
	vid->DrawCircle(center.X + value.offset.X, center.Y + value.offset.Y, radius / 4, radius / 4, COLR(color), COLG(color), COLB(color), COLA(color));
}

void DirectionSelector::OnMouseMoved(int x, int y, Point difference)
{
	if (mouseDown)
	{
		SetPositionAbs({ x, y });
	}
	CheckHovering(x, y);
}

void DirectionSelector::OnMouseDown(int x, int y, unsigned char button)
{
	mouseDown = true;
	SetPositionAbs({ x, y });
	CheckHovering(x, y);
}

void DirectionSelector::OnMouseUp(int x, int y, unsigned char button)
{
	mouseDown = false;
	if (autoReturn)
		SetPosition({ 0, 0 });
	CheckHovering(x - position.X, y - position.Y);

	if (changeCallback)
	{
		changeCallback(GetXValue(), GetYValue());
	}
}
