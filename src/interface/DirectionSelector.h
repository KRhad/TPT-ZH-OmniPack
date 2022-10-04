#ifndef DIRECTIONSELECTOR_H_
#define DIRECTIONSELECTOR_H_

#include "Component.h"
#include "common/Point.h"
#include "graphics/ARGBColour.h"

#include <iostream>

#include <cmath>
#include <vector>
#include <functional>

namespace gfx
{
class VideoBuffer;
}
class DirectionSelector : public Component
{
	const float scale;
	const float radius;
	const float handleRadius;

	bool useSnapPoints;
	int snapPointRadius;
	int snapPointEffectRadius;

	struct Value
	{
		Point offset;
		float xValue;
		float yValue;
	};
	std::vector<Value> snapPoints;

	bool autoReturn;

	ARGBColour backgroundColor;

public:
	using DirectionSelectorCallback = std::function<void(float x, float y)>;

private:
	DirectionSelectorCallback updateCallback;
	DirectionSelectorCallback changeCallback;

	bool mouseDown;
	bool mouseHover;
	bool altDown;

	Value value;

	void CheckHovering(int x, int y);

	Value GravityValueToValue(float x, float y);
	Value PositionToValue(Point position);

public:
	DirectionSelector(Point position, float scale, int radius, int handleRadius, int snapPointRadius, int snapPointEffectRadius);
	virtual ~DirectionSelector() = default;

	void SetSnapPoints(int newSnapPointEffectRadius, int points, float maxMagnitude);
	void ClearSnapPoints();

	inline void EnableSnapPoints() { useSnapPoints = true; }
	inline void DisableSnapPoints() { useSnapPoints = false; }

	inline void EnableAutoReturn() { autoReturn = true; }
	inline void DisableAutoReturn() { autoReturn = false; }

	inline void SetBackgroundColor(ARGBColour color) { backgroundColor = color; }

	float GetXValue();
	float GetYValue();

	void SetPositionAbs(Point position);
	void SetPosition(Point position);
	void SetValues(float x, float y);

	void OnDraw(gfx::VideoBuffer* vid) override;
	void OnMouseDown(int x, int y, unsigned char button) override;
	void OnMouseUp(int x, int y, unsigned char button) override;
	void OnMouseMoved(int x, int y, Point difference) override;
	inline void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override { altDown = alt; }
	inline void OnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override { altDown = alt; }

	inline void SetUpdateCallback(DirectionSelectorCallback callback) { updateCallback = callback; }
	inline void SetChangeCallback(DirectionSelectorCallback callback) { changeCallback = callback; }
};

#endif /* DIRECTIONSELECTOR_H_ */

