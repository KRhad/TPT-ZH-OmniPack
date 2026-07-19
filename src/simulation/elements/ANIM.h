#ifndef NOMOD
#ifndef ANIM_H
#define ANIM_H

#include <vector>
#include "common/tpt-minmax.h"
#include "graphics/ARGBColour.h"
#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"
#include "simulation/SnapshotDelta.h"

class ANIM_ElementDataContainer : public ElementDataContainer
{
	ARGBColour *animations[NPART];
	unsigned int maxFrames;

public:
	ANIM_ElementDataContainer()
	{
		for (int i = 0; i < NPART; i++)
			animations[i] = nullptr;
		maxFrames = 25;
	}

	virtual ~ANIM_ElementDataContainer()
	{
		for (int i = 0; i < NPART; i++)
			if (animations[i])
				delete[] animations[i];
	}

	std::unique_ptr<ElementDataContainer> Clone() override
	{
		std::unique_ptr<ANIM_ElementDataContainer> animContainer = std::make_unique<ANIM_ElementDataContainer>();
		animContainer->maxFrames = maxFrames;
		for (int i = 0; i < NPART; i++)
		{
			if (!animations[i])
				continue;
			ARGBColour *animationsClone = new ARGBColour[maxFrames];
			std::copy(&animations[i][0], &animations[i][maxFrames], &animationsClone[0]);
			animContainer->animations[i] = animationsClone;
		}

		return animContainer;
	}

	void Simulation_Cleared(Simulation *sim) override
	{
		for (int i = 0; i < NPART; i++)
			if (animations[i] != nullptr)
			{
				delete[] animations[i];
				animations[i] = nullptr;
			}
	}

	unsigned int GetMaxFrames()
	{
		return maxFrames;
	}

	void SetMaxFrames(unsigned int maxFrames)
	{
		this->maxFrames = maxFrames;
	}

	void InitlializePart(int i)
	{
		animations[i] = new ARGBColour[maxFrames];
		if (!animations[i])
			return;
		std::fill(&animations[i][0], &animations[i][maxFrames], 0);
	}

	void FreePart(int i)
	{
		if (animations[i])
		{
			delete animations[i];
			animations[i] = nullptr;
		}
	}

	bool isValid(int i, int tmp2)
	{
		if (!animations[i] || tmp2 < 0 || (unsigned int)tmp2 >= maxFrames)
			return false;
		return true;
	}

	ARGBColour GetColor(int i, int tmp2)
	{
		if (!animations[i] || tmp2 < 0 || (unsigned int)tmp2 >= maxFrames)
			return 0;
		return animations[i][tmp2];
	}

	std::vector<ARGBColour> GetAllColors(int i, int maxLength)
	{
		std::vector<ARGBColour> colors;
		for (unsigned int j = 0; j < (unsigned int)maxLength && j < maxFrames; j++)
			colors.push_back(animations[i][j]);
		return colors;
	}

	void SetColor(int i, int tmp2, ARGBColour color)
	{
		if (!animations[i] || tmp2 < 0 || (unsigned int)tmp2 >= maxFrames)
			return;
		animations[i][tmp2] = color;
	}

	void SetAllColors(int i, std::vector<ARGBColour> colors, int maxLength)
	{
		int animLen = tpt::min<int>(maxLength, (int)maxFrames);
		parts[i].ctype = animLen - 1;
		animations[i] = new ARGBColour[maxFrames];
		if (animations[i] == NULL)
			return;

		for (unsigned int j = 0; j < maxFrames; j++)
		{
			// Read animation data
			if (j < (unsigned int)maxLength && j < colors.size())
				animations[i][j] = colors[j];
			// Set the rest to 0
			else
				animations[i][j] = 0;
		}
	}

	void NewFrame(Simulation *sim, bool doCopy)
	{
		unsigned int framenum;
		bool canCopy = true, gotFrameNum = false;
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				if (!gotFrameNum)
				{
					framenum = parts[i].tmp2 + 1;
					if (framenum >= maxFrames)
					{
						canCopy = false;
						framenum = maxFrames - 1;
					}
					gotFrameNum = true;
				}
				parts[i].tmp = 0;
				parts[i].tmp2 = framenum;
				if (framenum > (unsigned int)parts[i].ctype)
					parts[i].ctype = framenum;

				if (doCopy && canCopy)
					animations[i][framenum] = animations[i][framenum-1];
				parts[i].dcolour = GetColor(i, framenum);
			}
	}

	void PreviousFrame(Simulation *sim)
	{
		unsigned int framenum;
		bool gotFrameNum = false;
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				if (!gotFrameNum)
				{
					if (parts[i].tmp2 <= 0)
						framenum = 0;
					else
						framenum = parts[i].tmp2 - 1;
					gotFrameNum = true;
				}
				parts[i].tmp = 0;
				parts[i].tmp2 = framenum;
				parts[i].dcolour = GetColor(i, framenum);
			}
	}

	void DeleteFrame(Simulation *sim)
	{
		unsigned int framenum;
		bool gotFrameNum = false;
		for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				if (!gotFrameNum)
				{
					if (parts[i].tmp2 < 0)
						framenum = 0;
					else
						framenum = parts[i].tmp2;
					gotFrameNum = true;
				}
				for (unsigned int j = framenum; j < maxFrames - 1; j++)
					animations[i][j] = animations[i][j+1];
				if (parts[i].ctype >= (int)framenum && parts[i].ctype)
					parts[i].ctype--;
				if (parts[i].tmp2 > parts[i].ctype)
					parts[i].tmp2 = parts[i].ctype;
				parts[i].dcolour = GetColor(i, parts[i].tmp2);
			}
	}
};

#endif
#endif
