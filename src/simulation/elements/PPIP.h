#ifndef PPIP_H
#define PPIP_H

#include "simulation/ElementDataContainer.h"
#include "powder.h"

// 0x00000100 is single pixel pipe
// 0x00000200 will transfer like a single pixel pipe when in forward mode
// 0x00001C00 forward single pixel pipe direction
// 0x00002000 will transfer like a single pixel pipe when in reverse mode
// 0x0001C000 reverse single pixel pipe direction
// 0x000E0000 PIPE color data stored here

constexpr int PFLAG_CAN_CONDUCT   = 0x00000001;
constexpr int PFLAG_PARTICLE_DECO = 0x00000002; // differentiate particle deco from pipe deco
constexpr int PFLAG_NORMALSPEED   = 0x00010000;
constexpr int PFLAG_INITIALIZING  = 0x00020000; // colors haven't been set yet
constexpr int PFLAG_COLOR_RED     = 0x00040000;
constexpr int PFLAG_COLOR_GREEN   = 0x00080000;
constexpr int PFLAG_COLOR_BLUE    = 0x000C0000;
constexpr int PFLAG_COLORS        = 0x000C0000;

constexpr int PPIP_TMPFLAG_REVERSED        = 0x01000000;
constexpr int PPIP_TMPFLAG_PAUSED          = 0x02000000;
constexpr int PPIP_TMPFLAG_TRIGGER_REVERSE = 0x04000000;
constexpr int PPIP_TMPFLAG_TRIGGER_OFF     = 0x08000000;
constexpr int PPIP_TMPFLAG_TRIGGER_ON      = 0x10000000;
constexpr int PPIP_TMPFLAG_TRIGGERS        = 0x1C000000;

class Simulation;

class PPIP_ElementDataContainer : public ElementDataContainer
{
public:
	bool ppip_changed;
	PPIP_ElementDataContainer()
	{
		ppip_changed = false;
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<PPIP_ElementDataContainer>(*this); }

	void Simulation_BeforeUpdate(Simulation *sim) override;
};

void PIPE_patchR(particle &part);
void PIPE_patchH(particle &part);
void PIPE_patchV(particle &part);
void PIPE_transfer_pipe_to_part(Simulation *sim, particle *pipe, particle *part, bool STOR=false);

#endif
