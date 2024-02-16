#ifndef ETRD_H
#define ETRD_H

#include <algorithm>
#include <cmath>
#include <vector>
#include "common/Point.h"
#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"


class ETRD_deltaWithLength
{
public:
	ETRD_deltaWithLength(Point a, int b):
		d(a),
		length(b)
	{

	}

	Point d;
	int length;
};


class ETRD_ElementDataContainer : public ElementDataContainer
{
public:
	std::vector<ETRD_deltaWithLength> deltaPos;
	static const int maxLength = 12;
	bool isValid;
	int countLife0;

	ETRD_ElementDataContainer()
	{
		invalidate();
		initDeltaPos();
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<ETRD_ElementDataContainer>(*this); }

	void invalidate()
	{
		isValid = false;
		countLife0 = 0;
	}

	void Simulation_Cleared(Simulation *sim) override
	{
		invalidate();
	}

	void Simulation_BeforeUpdate(Simulation *sim) override
	{
		invalidate();
	}

	void Simulation_AfterUpdate(Simulation *sim) override
	{
		invalidate();
	}

private:
	static bool compareFunc(const ETRD_deltaWithLength &a, const ETRD_deltaWithLength &b)
	{
		return a.length < b.length;
	}

	void initDeltaPos()
	{
		deltaPos.clear();
		for (int ry = -maxLength; ry <= maxLength; ry++)
			for (int rx = -maxLength; rx <= maxLength; rx++)
			{
				Point d(rx, ry);
				if (std::hypot(d.X, d.Y) <= maxLength)
					deltaPos.push_back(ETRD_deltaWithLength(d, std::abs(d.X) + std::abs(d.Y)));
			}
		std::stable_sort(deltaPos.begin(), deltaPos.end(), &ETRD_ElementDataContainer::compareFunc);
	}
};

int nearestSparkablePart(Simulation *sim, int targetId);

#endif
