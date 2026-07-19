#ifndef EMP_H
#define EMP_H

#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"

class EMP_ElementDataContainer : public ElementDataContainer
{
public:
	unsigned int emp_decor;
	unsigned int emp_trigger_count;
	EMP_ElementDataContainer()
	{
		emp_decor = 0;
		emp_trigger_count = 0;
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<EMP_ElementDataContainer>(*this); }

	void Simulation_Cleared(Simulation *sim) override
	{
		emp_decor = 0;
		emp_trigger_count = 0;
	}

	void Simulation_AfterUpdate(Simulation *sim) override;

	void Activate()
	{
		emp_trigger_count++;
		emp_decor += 3;
		if (emp_decor > 40)
			emp_decor = 40;
	}

	void Deactivate()
	{
		if (emp_decor)
			emp_decor -= emp_decor/25+2;

		if (emp_decor > 40)
			emp_decor = 40;
	}
};

#endif
