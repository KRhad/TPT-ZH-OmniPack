#ifndef LIFE_H
#define LIFE_H

#include <algorithm>
#include <string>
#include "simulation/ElementDataContainer.h"
#include "simulation/GolNumbers.h"
#include "simulation/GOLString.h"
#include "simulation/Simulation.h"
#include "simulation/SnapshotDelta.h"
#include "simulation/WallNumbers.h"

struct CustomGOLData
{
	int rule, color1, color2;
	std::string nameString, ruleString;

	inline bool operator ==(const CustomGOLData &other) const
	{
		return rule == other.rule;
	}
};

struct LIFE_ElementDataContainerDelta : ElementDataContainerDelta
{
	SnapshotDelta::SingleDiff<int> golGeneration;
};

class LIFE_ElementDataContainer : public ElementDataContainer
{
	unsigned int gol[YRES][XRES][5];
	int golSpeedCounter;

	std::vector<CustomGOLData> customGol;
	std::string cachedName = "CGOL";
	std::string cachedRuleString = "B3/S23";

public:
	int golSpeed;
	int golGeneration;
	LIFE_ElementDataContainer()
	{
		std::fill_n(&gol[0][0][0], YRES*XRES*5, 0);
		golSpeed = 1;
		golSpeedCounter = 0;
		golGeneration = 0;
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<LIFE_ElementDataContainer>(*this); }

	void Simulation_Cleared(Simulation *sim) override
	{
		std::fill_n(&gol[0][0][0], YRES*XRES*5, 0);
		golSpeedCounter = 0;
		golGeneration = 0;
	}

	void Simulation_BeforeUpdate(Simulation *sim) override
	{
		//golSpeed is frames per generation
		if (sim->elementCount[PT_LIFE] <= 0 || ++golSpeedCounter < golSpeed)
			return;

		bool createdSomething = false;
		golSpeedCounter = 0;

		for (int i = 0; i <= sim->parts_lastActiveIndex; ++i)
		{
			auto &part = parts[i];
			if (part.type != PT_LIFE)
			{
				continue;
			}
			auto x = int(part.x + 0.5f);
			auto y = int(part.y + 0.5f);
			if (x < CELL || y < CELL || x >= XRES - CELL || y >= YRES - CELL)
			{
				continue;
			}
			unsigned int golnum = part.ctype;
			unsigned int ruleset = golnum;
			if (golnum < NGOL)
			{
				ruleset = builtinGol[golnum].ruleset;
				golnum += 1;
			}
			if (part.tmp2 == int((ruleset >> 17) & 0xF) + 1)
			{
				for (int yy = -1; yy <= 1; ++yy)
				{
					for (int xx = -1; xx <= 1; ++xx)
					{
						if (xx || yy)
						{
							// * Calculate address of the neighbourList, taking wraparound
							//   into account. The fact that the GOL space is 2 CELL's worth
							//   narrower in both dimensions than the simulation area makes
							//   this a bit awkward.
							int ax = ((x + xx + XRES - 3 * CELL) % (XRES - 2 * CELL)) + CELL;
							int ay = ((y + yy + YRES - 3 * CELL) % (YRES - 2 * CELL)) + CELL;
							if (pmap[ay][ax] && TYP(pmap[ay][ax]) != PT_LIFE)
							{
								continue;
							}
							unsigned int (&neighbourList)[5] = gol[ay][ax];
							// * Bump overall neighbour counter (bits 30..28) for the entire list.
							neighbourList[0] += 1U << 28;
							for (int l = 0; l < 5; ++l)
							{
								auto neighbourRuleset = neighbourList[l] & 0x001FFFFFU;
								if (neighbourRuleset == golnum)
								{
									// * Bump population counter (bits 23..21) of the
									//   same kind of cell.
									neighbourList[l] += 1U << 21;
									break;
								}
								if (neighbourRuleset == 0)
								{
									// * Add the new kind of cell to the population. Both counters
									//   have a bias of -1, so they're intentionally initialised
									//   to 0 instead of 1 here. This is all so they can both
									//   fit in 3 bits.
									neighbourList[l] = ((yy & 3) << 26) | ((xx & 3) << 24) | golnum;
									break;
								}
								// * If after 5 iterations the cell still hasn't contributed
								//   to a list entry, it's surely a 6th kind of cell, meaning
								//   there could be at most 3 of it in the neighbourhood,
								//   as there are already 5 other kinds of cells present in
								//   the list. This in turn means that it couldn't possibly
								//   win the population ratio-based contest later on.
							}
						}
					}
				}
			}
			else
			{
				if (!(bmap[y / CELL][x / CELL] == WL_STASIS && emap[y / CELL][x / CELL] < 8))
				{
					part.tmp2 -= 1;
				}
			}
		}
		for (int y = CELL; y < YRES - CELL; ++y)
		{
			for (int x = CELL; x < XRES - CELL; ++x)
			{
				int r = pmap[y][x];
				if (r && TYP(r) != PT_LIFE)
				{
					continue;
				}
				unsigned int (&neighbourList)[5] = gol[y][x];
				auto nl0 = neighbourList[0];
				if (r || nl0)
				{
					// * Get overall neighbour count (bits 30..28).
					unsigned int neighbours = nl0 ? ((nl0 >> 28) & 7) + 1 : 0;
					if (!(bmap[y / CELL][x / CELL] == WL_STASIS && emap[y / CELL][x / CELL] < 8))
					{
						if (r)
						{
							auto &part = parts[ID(r)];
							unsigned int ruleset = part.ctype;
							if (ruleset < NGOL)
							{
								ruleset = builtinGol[ruleset].ruleset;
							}
							if (!((ruleset >> neighbours) & 1) && part.tmp2 == int(ruleset >> 17) + 1)
							{
								// * Start death sequence.
								part.tmp2 -= 1;
							}
						}
						else
						{
							unsigned int golnumToCreate = 0xFFFFFFFFU;
							unsigned int createFromEntry = 0U;
							unsigned int majority = neighbours / 2 + neighbours % 2;
							for (int l = 0; l < 5; ++l)
							{
								auto golnum = neighbourList[l] & 0x001FFFFFU;
								if (!golnum)
								{
									break;
								}
								auto ruleset = golnum;
								if (golnum - 1 < NGOL)
								{
									ruleset = builtinGol[golnum - 1].ruleset;
									golnum -= 1;
								}
								if ((ruleset >> (neighbours + 8)) & 1 && ((neighbourList[l] >> 21) & 7) + 1 >= majority && golnum < golnumToCreate)
								{
									golnumToCreate = golnum;
									createFromEntry = neighbourList[l];
								}
							}
							if (golnumToCreate != 0xFFFFFFFFU)
							{
								// * 0x200000: No need to look for colours, they'll be set later anyway.
								int i = sim->part_create(-1, x, y, PT_LIFE, golnumToCreate | 0x200000);
								if (i >= 0)
								{
									createdSomething = true;
									int xx = (createFromEntry >> 24) & 3;
									int yy = (createFromEntry >> 26) & 3;
									if (xx == 3) xx = -1;
									if (yy == 3) yy = -1;
									int ax = ((x - xx + XRES - 3 * CELL) % (XRES - 2 * CELL)) + CELL;
									int ay = ((y - yy + YRES - 3 * CELL) % (YRES - 2 * CELL)) + CELL;
									auto &sample = parts[ID(pmap[ay][ax])];
									parts[i].dcolour = sample.dcolour;
									parts[i].tmp = sample.tmp;
								}
							}
						}
					}
					for (int l = 0; l < 5 && neighbourList[l]; ++l)
					{
						neighbourList[l] = 0;
					}
				}
			}
		}
		for (int y = CELL; y < YRES - CELL; ++y)
		{
			for (int x = CELL; x < XRES - CELL; ++x)
			{
				int r = pmap[y][x];
				if (r && TYP(r) == PT_LIFE && parts[ID(r)].tmp2 <= 0)
				{
					sim->part_kill(ID(r));
				}
			}
		}

		if (createdSomething)
			golGeneration++;
	}

	const CustomGOLData *GetCustomGOLByRule(int rule) const
	{
		auto it = std::find_if(customGol.begin(), customGol.end(), [&](CustomGOLData c) { return c.rule == rule; });
		if (it != customGol.end())
			return &*it;
		return nullptr;
	}

	const CustomGOLData *GetCustomGOLByName(std::string name) const
	{
		auto it = std::find_if(customGol.begin(), customGol.end(), [&](CustomGOLData c) { return c.nameString == name; });
		if (it != customGol.end())
			return &*it;
		return nullptr;
	}

	const std::vector<CustomGOLData> GetCustomGOL()
	{
		return customGol;
	}

	bool AddCustomGOL(CustomGOLData cgol)
	{
		if (std::any_of(customGol.begin(), customGol.end(), [&](CustomGOLData c) { return cgol.nameString == c.nameString || cgol.rule == c.rule; }))
			return false;

		customGol.push_back(cgol);
		return true;
	}

	void RemoveCustomGOL(int rule)
	{
		const CustomGOLData *cgol = GetCustomGOLByRule(rule);
		customGol.erase(std::remove_if(customGol.begin(), customGol.end(), [&](CustomGOLData c) { return cgol->rule == c.rule; }), customGol.end());
	}

	void SetCachedName(std::string name)
	{
		cachedName = name;
	}

	void SetCachedRuleString(std::string ruleString)
	{
		cachedRuleString = ruleString;
	}

	std::string GetCachedName()
	{
		return cachedName;
	}

	std::string GetCachedRuleString()
	{
		return cachedRuleString;
	}

	std::unique_ptr<ElementDataContainerDelta> Compare(const std::unique_ptr<ElementDataContainer> &o, const std::unique_ptr<ElementDataContainer> &n) override
	{
		std::unique_ptr<LIFE_ElementDataContainerDelta> delta = std::make_unique<LIFE_ElementDataContainerDelta>(LIFE_ElementDataContainerDelta());
		LIFE_ElementDataContainer *oldContainer = &static_cast<LIFE_ElementDataContainer&>(*o);
		LIFE_ElementDataContainer *newContainer = &static_cast<LIFE_ElementDataContainer&>(*n);

		SnapshotDelta::FillSingleDiff<int>(oldContainer->golGeneration, newContainer->golGeneration, delta->golGeneration);
		return delta;
	}

	void Forward(const std::unique_ptr<ElementDataContainer> &prti_datacontainer, const std::unique_ptr<ElementDataContainerDelta> &life_delta) override
	{
		LIFE_ElementDataContainer *container = &static_cast<LIFE_ElementDataContainer&>(*prti_datacontainer);
		LIFE_ElementDataContainerDelta *delta = &static_cast<LIFE_ElementDataContainerDelta&>(*life_delta);
		SnapshotDelta::ApplySingleDiff<false>(delta->golGeneration, container->golGeneration);
	}

	void Restore(const std::unique_ptr<ElementDataContainer> &prti_datacontainer, const std::unique_ptr<ElementDataContainerDelta> &life_delta) override
	{
		LIFE_ElementDataContainer *container = &static_cast<LIFE_ElementDataContainer&>(*prti_datacontainer);
		LIFE_ElementDataContainerDelta *delta = &static_cast<LIFE_ElementDataContainerDelta&>(*life_delta);
		SnapshotDelta::ApplySingleDiff<true>(delta->golGeneration, container->golGeneration);
	}
};

#endif
