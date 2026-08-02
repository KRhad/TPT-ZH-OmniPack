#include "simulation/ElementClasses.h"
#include "simulation/ElementDefs.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

#include <array>
#include <iostream>
#include <string_view>
#include <utility>

namespace
{
int Fail(std::string_view message, int movingType = PT_NONE, int destinationType = PT_NONE)
{
	std::cerr << "solid-barrier-probe: FAIL " << message;
	if (movingType != PT_NONE || destinationType != PT_NONE)
	{
		std::cerr << " moving_id=" << movingType
			<< " destination_id=" << destinationType;
	}
	std::cerr << std::endl;
	return 1;
}

bool IsIntentionalException(int movingType, int destinationType)
{
	if (destinationType == PT_BHOL || destinationType == PT_NBHL ||
		destinationType == PT_INVIS || destinationType == PT_PVOD ||
		destinationType == PT_VOID)
	{
		return true;
	}
	return movingType == PT_ANAR &&
		(destinationType == PT_WHOL || destinationType == PT_NWHL);
}
}

int main()
{
	SimulationData simulationData;
	auto const &elements = simulationData.elements;
	auto const &canMove = simulationData.can_move;
	int checkedPairs = 0;

	for (int movingType = 1; movingType < PT_NUM; ++movingType)
	{
		if (!elements[movingType].Enabled || !(elements[movingType].Properties & TYPE_PART))
			continue;
		for (int destinationType = 1; destinationType < PT_NUM; ++destinationType)
		{
			if (!elements[destinationType].Enabled ||
				!(elements[destinationType].Properties & TYPE_SOLID) ||
				IsIntentionalException(movingType, destinationType))
			{
				continue;
			}
			++checkedPairs;
			if (canMove[movingType][destinationType] != 0)
				return Fail("powder can displace solid", movingType, destinationType);
		}
	}

	constexpr std::array representativePairs{
		std::pair{ PT_DUST, PT_AERG },
		std::pair{ PT_SAND, PT_GRPH },
		std::pair{ PT_DUST, PT_CFRP },
	};
	for (auto [movingType, destinationType] : representativePairs)
	{
		if (canMove[movingType][destinationType] != 0)
			return Fail("representative barrier pair is not blocked", movingType, destinationType);
	}
	if (canMove[PT_DUST][PT_NONE] != 1)
		return Fail("dust no longer moves into empty space", PT_DUST, PT_NONE);
	if (canMove[PT_SAND][PT_WATR] != 1)
		return Fail("density swap between movable materials changed", PT_SAND, PT_WATR);
	if (canMove[PT_DUST][PT_BHOL] != 1 || canMove[PT_DUST][PT_VOID] != 3 ||
		canMove[PT_DUST][PT_INVIS] != 3 || canMove[PT_ANAR][PT_WHOL] != 1)
	{
		return Fail("intentional special passage changed");
	}
	if (checkedPairs == 0)
		return Fail("no powder-solid pairs were checked");

	auto simulation = Simulation::Factory();
	constexpr int probeX = 100;
	constexpr int solidY = 100;
	constexpr int powderY = solidY - 1;
	auto solidIndex = simulation->create_part(-1, probeX, solidY, PT_AERG);
	auto powderIndex = simulation->create_part(-1, probeX, powderY, PT_DUST);
	if (solidIndex < 0 || powderIndex < 0)
		return Fail("could not create runtime barrier particles");
	if (simulation->do_move(powderIndex, probeX, powderY, float(probeX), float(solidY)) != 0)
		return Fail("runtime dust movement entered aerogel", PT_DUST, PT_AERG);
	if (TYP(simulation->pmap[solidY][probeX]) != PT_AERG ||
		TYP(simulation->pmap[powderY][probeX]) != PT_DUST)
	{
		return Fail("runtime barrier particle positions changed", PT_DUST, PT_AERG);
	}

	std::cout << "solid_barrier_probe_pass=true\n";
	std::cout << "powder_solid_pairs_checked=" << checkedPairs << '\n';
	return 0;
}
