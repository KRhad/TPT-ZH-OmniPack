#ifndef PPIP_H
#define PPIP_H

#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"

class PPIP_ElementDataContainer : public ElementDataContainer
{
public:
	bool ppip_changed;
	PPIP_ElementDataContainer()
	{
		ppip_changed = false;
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<PPIP_ElementDataContainer>(*this); }

	void Simulation_BeforeUpdate(Simulation *sim) override
	{
		if (ppip_changed)
		{
			for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
			{
				if (parts[i].type == PT_PPIP)
				{
					parts[i].tmp |= (parts[i].tmp&0xE0000000)>>3;
					parts[i].tmp &= ~0xE0000000;
				}
			}
			ppip_changed = false;
		}
	}
};

void PIPE_patchR(particle &part);
void PIPE_patchH(particle &part);
void PIPE_patchV(particle &part);
void PIPE_transfer_pipe_to_part(Simulation *sim, particle *pipe, particle *part, bool STOR=false);

#endif
