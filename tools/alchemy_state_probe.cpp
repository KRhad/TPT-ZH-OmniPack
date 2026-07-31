#include "client/GameSave.h"
#include "simulation/OmniAlchemy.h"
#include "simulation/SimulationData.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
int Fail(char const *message)
{
	std::cerr << "alchemy-state-probe: FAIL " << message << std::endl;
	return 1;
}

bool Has(std::vector<ByteString> const &values, char const *value)
{
	return std::find(values.begin(), values.end(), value) != values.end();
}
}

int main()
{
	SimulationData simulationData;
	auto &alchemy = OmniAlchemy::Ref();
	alchemy.Reset();
	if (!alchemy.IsIdentifierUnlocked("DEFAULT_PT_FIRE") ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_WATR") ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_STNE") ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_O2") ||
		alchemy.IsIdentifierUnlocked("DEFAULT_PT_METL"))
	{
		return Fail("initial unlock set differs from FIRE/WATR/STNE/O2");
	}

	auto blankProgress = alchemy.Export();
	auto stageOne = blankProgress;
	stageOne.unlockedIdentifiers.emplace_back("DEFAULT_PT_WTRV");
	stageOne.completedStages.emplace_back("A01-STEAM-CYCLE");
	stageOne.records = stageOne.completedStages;
	if (!alchemy.Import(stageOne) || alchemy.CompletedStageCount() != 1 ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_WTRV") ||
		alchemy.IsIdentifierUnlocked("DEFAULT_PT_METL"))
	{
		return Fail("valid stage-one identifier state was not restored");
	}

	GameSave save(Vec2<int>{ 1, 1 });
	save.omniAlchemy = alchemy.Export();
	auto serialised = save.Serialise().second;
	GameSave loaded(serialised);
	if (!loaded.omniAlchemy.present || !loaded.omniAlchemy.valid ||
		loaded.omniAlchemy.schemaVersion != OmniAlchemyProgressSchemaVersion ||
		loaded.omniAlchemy.completedStages != stageOne.completedStages ||
		!Has(loaded.omniAlchemy.unlockedIdentifiers, "DEFAULT_PT_WTRV"))
	{
		return Fail("OPS omniAlchemy roundtrip did not preserve stage one");
	}

	if (!alchemy.Import(blankProgress) || alchemy.CompletedStageCount() != 0 ||
		alchemy.IsIdentifierUnlocked("DEFAULT_PT_WTRV"))
	{
		return Fail("second save did not restore an isolated blank progress state");
	}
	if (!alchemy.Import(loaded.omniAlchemy) || alchemy.CompletedStageCount() != 1 ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_WTRV"))
	{
		return Fail("first save progress was not isolated from the second save");
	}

	auto mastery = blankProgress;
	for (auto stage : {
		"A01-STEAM-CYCLE",
		"A02-THERMAL-CYCLE",
		"A03-CONTAINED-PRESSURE",
		"A04-CLOSED-CIRCUIT",
		"A05-CATALYTIC-ELECTROLYSIS",
		"A06-GRANULAR-FILTRATION",
		"A07-INDUSTRIAL-FURNACE",
		"A08-SELECTIVE-CHEMISTRY",
		"A09-CONTROLLED-TRANSFER",
		"A10-MASTERY-LOOP",
	})
	{
		mastery.completedStages.emplace_back(stage);
	}
	mastery.records = mastery.completedStages;
	for (auto identifier : {
		"DEFAULT_PT_WTRV",
		"DEFAULT_PT_LAVA", "DEFAULT_PT_SAND", "DEFAULT_PT_BRCK",
		"DEFAULT_PT_METL", "DEFAULT_PT_BTRY", "DEFAULT_PT_PUMP",
		"DEFAULT_PT_SPRK", "DEFAULT_PT_PSCN", "DEFAULT_PT_NSCN",
		"DEFAULT_PT_H2", "DEFAULT_PT_ACID",
		"DEFAULT_PT_FILT", "DEFAULT_PT_PIPE", "DEFAULT_PT_GLAS",
		"DEFAULT_PT_CRMC", "DEFAULT_PT_COAL", "OMNI_PT_ALUM", "OMNI_PT_COPR", "OMNI_PT_TIN", "OMNI_PT_NICL",
		"DEFAULT_PT_SALT", "OMNI_PT_FERT",
		"DEFAULT_PT_INST", "DEFAULT_PT_CONV",
		"DEFAULT_PT_DMND",
	})
	{
		mastery.unlockedIdentifiers.emplace_back(identifier);
	}
	if (!alchemy.Import(mastery) || !alchemy.Mastered() || alchemy.CompletedStageCount() != 10 ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_DUST"))
	{
		return Fail("canonical ten-stage mastery state was not accepted");
	}
	GameSave masterySave(Vec2<int>{ 1, 1 });
	masterySave.omniAlchemy = alchemy.Export();
	GameSave masteryLoaded(masterySave.Serialise().second);
	for (int iteration = 0; iteration < 100; ++iteration)
	{
		if (!alchemy.Import(masteryLoaded.omniAlchemy) || !alchemy.Mastered() ||
			alchemy.Export().completedStages.size() != OmniAlchemyStageCount ||
			alchemy.Export().records.size() != OmniAlchemyStageCount)
		{
			return Fail("repeated mastery load introduced loss or duplicate records");
		}
	}

	auto corrupt = loaded.omniAlchemy;
	corrupt.unlockedIdentifiers.emplace_back("DEFAULT_PT_DMND");
	if (alchemy.Import(corrupt) || alchemy.CompletedStageCount() != 0 ||
		!alchemy.IsIdentifierUnlocked("DEFAULT_PT_FIRE") ||
		alchemy.IsIdentifierUnlocked("DEFAULT_PT_DMND"))
	{
		return Fail("inconsistent progress did not fail closed to the initial state");
	}

	auto unknownSchema = blankProgress;
	unknownSchema.schemaVersion = 99;
	if (alchemy.Import(unknownSchema) || alchemy.CompletedStageCount() != 0)
	{
		return Fail("unknown schema was accepted");
	}

	std::cout
		<< "alchemy-state-probe: PASS "
		<< "initial=4 stage_roundtrip=10 repeated_loads=100 multi_save_isolation=true "
		<< "corrupt_fail_closed=true unknown_schema_rejected=true mastery=true"
		<< std::endl;
	return 0;
}
