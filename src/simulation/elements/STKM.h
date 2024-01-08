#ifndef STKM_H
#define STKM_H

struct Stickman
{
	char comm;           //command cell
	char pcomm;          //previous command
	int elem;            //element power
	float legs[16];      //legs' positions
	float accs[8];       //accelerations
	char spwn;           //if stick man was spawned
	unsigned int frames; //frames since last particle spawn - used when spawning LIGH
	int spawnID;         //id of the SPWN particle
	char rocketBoots;
	bool fan;
};


#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"

class STKM_ElementDataContainer : public ElementDataContainer
{
	Stickman player;
	Stickman player2;

public:
	STKM_ElementDataContainer()
	{
		InitLegs(&player, -1);
		InitLegs(&player2, -1);
		player.spawnID = -1;
		player.elem = PT_DUST;
		player2.spawnID = -1;
		player2.elem = PT_DUST;
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<STKM_ElementDataContainer>(*this); }

	void Simulation_Cleared(Simulation *sim) override
	{
		InitLegs(&player, -1);
		InitLegs(&player2, -1);
		player.spawnID = -1;
		player.elem = PT_DUST;
		player2.spawnID = -1;
		player2.elem = PT_DUST;
	}

	void Simulation_BeforeUpdate(Simulation *sim) override
	{
		//create stickmen if the current one has been deleted
		if (sim->elementCount[PT_STKM] <= 0 && player.spawnID >= 0 && player.spawnID < NPART && sim->parts[player.spawnID].type == PT_SPAWN)
			sim->part_create(-1, (int)parts[player.spawnID].x, (int)parts[player.spawnID].y, PT_STKM);
		else if (sim->elementCount[PT_STKM2] <= 0 && player2.spawnID >= 0 && player2.spawnID < NPART && sim->parts[player2.spawnID].type == PT_SPAWN2)
			sim->part_create(-1, (int)parts[player2.spawnID].x, (int)parts[player2.spawnID].y, PT_STKM2);
	}

	void Simulation_AfterUpdate(Simulation *sim) override;

	Stickman * GetStickman1()
	{
		return &player;
	}
	Stickman * GetStickman2()
	{
		return &player2;
	}

	void NewStickman1(int i, int elem);
	void NewStickman2(int i, int elem);

	void InitLegs(Stickman *playerp, int i)
	{
		int x, y;
		if (i >= 0)
		{
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
		}
		else
			x = y = 0;

		playerp->legs[0] = x-1.0f;
		playerp->legs[1] = y+6.0f;
		playerp->legs[2] = x-1.0f;
		playerp->legs[3] = y+6.0f;

		playerp->legs[4] = x-3.0f;
		playerp->legs[5] = y+12.0f;
		playerp->legs[6] = x-3.0f;
		playerp->legs[7] = y+12.0f;

		playerp->legs[8] = x+1.0f;
		playerp->legs[9] = y+6.0f;
		playerp->legs[10] = x+1.0f;
		playerp->legs[11] = y+6.0f;

		playerp->legs[12] = x+3.0f;
		playerp->legs[13] = y+12.0f;
		playerp->legs[14] = x+3.0f;
		playerp->legs[15] = y+12.0f;

		for (int i = 0; i < 8; i++)
			playerp->accs[i] = 0;

		playerp->comm = 0;
		playerp->pcomm = 0;
		//playerp->elem = PT_DUST;
		playerp->spwn = 0;
		playerp->frames = 0;
		//playerp->spawnID = -1;
		playerp->rocketBoots = false;
		playerp->fan = false;
	}

	enum StkmKeys
	{
		None, Up, Left, Down, Right
	};

	void Die(Simulation *sim, Stickman *playerp, int i);
	int Run(Stickman *playerp, UPDATE_FUNC_ARGS);
	void Interact(Simulation *sim, Stickman *playerp, int i, int x, int y);
	void HandleKeyPress(StkmKeys key, bool stk2);
	void HandleKeyRelease(StkmKeys key, bool stk2);
	void STKM_default_element(Simulation *sim, Stickman *playerp);
	void STKM_set_element(Simulation *sim, Stickman *playerp, int element);
};

#endif
