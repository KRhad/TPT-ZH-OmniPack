#include "OmniAlchemy.h"

#include "simulation/ElementClasses.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace
{
constexpr int AlchemyScanIntervalFrames = 30;
constexpr int AlchemyScanParticleBudget = 8192;

struct StageDefinition
{
	char const *id = "";
	char const *noticeKey = "";
	char const *hintKey = "";
	char const *nameKey = "";
	std::array<int, 4> inputs{};
	std::size_t inputCount = 0;
	std::array<int, 6> outputs{};
	std::size_t outputCount = 0;
	int temperatureTarget = PT_NONE;
	float minimumTemperature = 0.0f;
	int pressureTarget = PT_NONE;
	float minimumAbsolutePressure = 0.0f;
	int electricTarget = PT_NONE;
	int minimumElectricCount = 0;
	int catalystTarget = PT_NONE;
	int minimumCatalystCount = 0;
	std::array<int, 4> structureTargets{};
	std::size_t structureTargetCount = 0;
	int structureRadius = 0;
	int coolingHeatTarget = PT_NONE;
	float coolingMinimumTemperature = 0.0f;
	int coolingCoolTarget = PT_NONE;
	float coolingMaximumTemperature = 0.0f;
	int filterMedium = PT_NONE;
	int filterFluid = PT_NONE;
	int filterMinimumCount = 0;
	int continuousFrames = 0;
	std::uint16_t prerequisiteMask = 0;
};

constexpr std::array<StageDefinition, OmniAlchemyStageCount> stages{ {
	{
		.id = "A01-STEAM-CYCLE",
		.noticeKey = "alchemy.stage.a01.notice",
		.hintKey = "alchemy.stage.a01.hint",
		.nameKey = "alchemy.stage.a01.name",
		.inputs = { PT_FIRE, PT_WATR }, .inputCount = 2,
		.outputs = { PT_WTRV }, .outputCount = 1,
		.temperatureTarget = PT_WTRV, .minimumTemperature = 373.15f,
		.continuousFrames = 120,
	},
	{
		.id = "A02-THERMAL-CYCLE",
		.noticeKey = "alchemy.stage.a02.notice",
		.hintKey = "alchemy.stage.a02.hint",
		.nameKey = "alchemy.stage.a02.name",
		.inputs = { PT_FIRE, PT_STNE, PT_WTRV }, .inputCount = 3,
		.outputs = { PT_LAVA, PT_SAND, PT_BRCK }, .outputCount = 3,
		.temperatureTarget = PT_LAVA, .minimumTemperature = 973.15f,
		.coolingHeatTarget = PT_LAVA, .coolingMinimumTemperature = 973.15f,
		.coolingCoolTarget = PT_STNE, .coolingMaximumTemperature = 500.0f,
		.prerequisiteMask = 1U << 0,
	},
	{
		.id = "A03-CONTAINED-PRESSURE",
		.noticeKey = "alchemy.stage.a03.notice",
		.hintKey = "alchemy.stage.a03.hint",
		.nameKey = "alchemy.stage.a03.name",
		.inputs = { PT_WTRV, PT_BRCK }, .inputCount = 2,
		.outputs = { PT_METL, PT_BTRY, PT_PUMP }, .outputCount = 3,
		.pressureTarget = PT_WTRV, .minimumAbsolutePressure = 1.5f,
		.structureTargets = { PT_WTRV, PT_BRCK }, .structureTargetCount = 2, .structureRadius = 12,
		.continuousFrames = 180,
	},
	{
		.id = "A04-CLOSED-CIRCUIT",
		.noticeKey = "alchemy.stage.a04.notice",
		.hintKey = "alchemy.stage.a04.hint",
		.nameKey = "alchemy.stage.a04.name",
		.inputs = { PT_BTRY, PT_METL }, .inputCount = 2,
		.outputs = { PT_SPRK, PT_PSCN, PT_NSCN }, .outputCount = 3,
		.electricTarget = PT_SPRK, .minimumElectricCount = 1,
		.structureTargets = { PT_BTRY, PT_METL, PT_SPRK }, .structureTargetCount = 3, .structureRadius = 8,
		.continuousFrames = 120,
	},
	{
		.id = "A05-CATALYTIC-ELECTROLYSIS",
		.noticeKey = "alchemy.stage.a05.notice",
		.hintKey = "alchemy.stage.a05.hint",
		.nameKey = "alchemy.stage.a05.name",
		.inputs = { PT_WATR, PT_O2, PT_SPRK, PT_METL }, .inputCount = 4,
		.outputs = { PT_H2, PT_ACID }, .outputCount = 2,
		.electricTarget = PT_SPRK, .minimumElectricCount = 1,
		.catalystTarget = PT_METL, .minimumCatalystCount = 4,
		.continuousFrames = 240,
	},
	{
		.id = "A06-GRANULAR-FILTRATION",
		.noticeKey = "alchemy.stage.a06.notice",
		.hintKey = "alchemy.stage.a06.hint",
		.nameKey = "alchemy.stage.a06.name",
		.inputs = { PT_WATR, PT_SAND, PT_BRCK }, .inputCount = 3,
		.outputs = { PT_FILT, PT_PIPE, PT_GLAS }, .outputCount = 3,
		.structureTargets = { PT_WATR, PT_SAND, PT_BRCK }, .structureTargetCount = 3, .structureRadius = 12,
		.filterMedium = PT_SAND, .filterFluid = PT_WATR, .filterMinimumCount = 4,
		.continuousFrames = 300,
	},
	{
		.id = "A07-INDUSTRIAL-FURNACE",
		.noticeKey = "alchemy.stage.a07.notice",
		.hintKey = "alchemy.stage.a07.hint",
		.nameKey = "alchemy.stage.a07.name",
		.inputs = { PT_LAVA, PT_PIPE, PT_METL }, .inputCount = 3,
		.outputs = { PT_CRMC, PT_COAL, PT_ALUM, PT_COPR, PT_TIN, PT_NICL }, .outputCount = 6,
		.temperatureTarget = PT_LAVA, .minimumTemperature = 1375.0f,
		.pressureTarget = PT_LAVA, .minimumAbsolutePressure = 2.0f,
		.catalystTarget = PT_FIRE, .minimumCatalystCount = 4,
		.prerequisiteMask = 1U << 5,
	},
	{
		.id = "A08-SELECTIVE-CHEMISTRY",
		.noticeKey = "alchemy.stage.a08.notice",
		.hintKey = "alchemy.stage.a08.hint",
		.nameKey = "alchemy.stage.a08.name",
		.inputs = { PT_ACID, PT_WATR, PT_FILT }, .inputCount = 3,
		.outputs = { PT_SALT, PT_FERT }, .outputCount = 2,
		.catalystTarget = PT_SAND, .minimumCatalystCount = 2,
		.coolingHeatTarget = PT_ACID, .coolingMinimumTemperature = 330.0f,
		.coolingCoolTarget = PT_WATR, .coolingMaximumTemperature = 295.0f,
		.filterMedium = PT_FILT, .filterFluid = PT_ACID, .filterMinimumCount = 2,
		.continuousFrames = 240,
	},
	{
		.id = "A09-CONTROLLED-TRANSFER",
		.noticeKey = "alchemy.stage.a09.notice",
		.hintKey = "alchemy.stage.a09.hint",
		.nameKey = "alchemy.stage.a09.name",
		.inputs = { PT_PUMP, PT_FILT, PT_PIPE, PT_SPRK }, .inputCount = 4,
		.outputs = { PT_INST, PT_CONV }, .outputCount = 2,
		.pressureTarget = PT_PUMP, .minimumAbsolutePressure = 2.0f,
		.electricTarget = PT_SPRK, .minimumElectricCount = 1,
		.structureTargets = { PT_PUMP, PT_FILT, PT_PIPE, PT_SPRK }, .structureTargetCount = 4, .structureRadius = 16,
		.continuousFrames = 360,
	},
	{
		.id = "A10-MASTERY-LOOP",
		.noticeKey = "alchemy.stage.a10.notice",
		.hintKey = "alchemy.stage.a10.hint",
		.nameKey = "alchemy.stage.a10.name",
		.inputs = { PT_H2, PT_O2, PT_CRMC, PT_INST }, .inputCount = 4,
		.outputs = { PT_DMND }, .outputCount = 1,
		.temperatureTarget = PT_FIRE, .minimumTemperature = 1000.0f,
		.pressureTarget = PT_H2, .minimumAbsolutePressure = 3.0f,
		.electricTarget = PT_SPRK, .minimumElectricCount = 1,
		.catalystTarget = PT_CRMC, .minimumCatalystCount = 4,
		.structureTargets = { PT_H2, PT_O2, PT_CRMC, PT_INST }, .structureTargetCount = 4, .structureRadius = 20,
		.coolingHeatTarget = PT_FIRE, .coolingMinimumTemperature = 1000.0f,
		.coolingCoolTarget = PT_CRMC, .coolingMaximumTemperature = 350.0f,
		.filterMedium = PT_FILT, .filterFluid = PT_H2, .filterMinimumCount = 2,
		.continuousFrames = 600,
		.prerequisiteMask = 1U << 8,
	},
} };

static_assert(stages.size() == OmniAlchemyStageCount);

ByteString CopyView(std::string_view value)
{
	return ByteString(value.begin(), value.end());
}

}

OmniAlchemy &OmniAlchemy::Ref()
{
	static OmniAlchemy instance;
	return instance;
}

OmniAlchemy::OmniAlchemy()
{
	Reset();
}

void OmniAlchemy::Reset()
{
	unlockedIdentifiers.clear();
	for (auto identifier : {
		"DEFAULT_PT_FIRE",
		"DEFAULT_PT_WATR",
		"DEFAULT_PT_STNE",
		"DEFAULT_PT_O2",
	})
	{
		unlockedIdentifiers.emplace(identifier);
	}
	completedStages.clear();
	records.clear();
	dwellFrames.fill(0);
	coolingArmed.fill(false);
	pendingNoticeKeys.clear();
	ResetTransientScan();
}

void OmniAlchemy::ResetTransientScan()
{
	scan = {};
}

void OmniAlchemy::BeginScan()
{
	scan = {};
	scan.active = true;
	scan.maximumTemperature.fill(-std::numeric_limits<float>::infinity());
	scan.minimumTemperature.fill(std::numeric_limits<float>::infinity());
}

bool OmniAlchemy::ValidateImportedState(OmniAlchemySaveState const &state) const
{
	if (!state.present || !state.valid || state.schemaVersion != OmniAlchemyProgressSchemaVersion)
	{
		return false;
	}
	if (state.unlockedIdentifiers.size() > OmniAlchemyMaxSavedIdentifiers ||
		state.completedStages.size() > OmniAlchemyStageCount ||
		state.records.size() > OmniAlchemyMaxSavedRecords)
	{
		return false;
	}

	std::set<ByteString> expected{
		"DEFAULT_PT_FIRE",
		"DEFAULT_PT_WATR",
		"DEFAULT_PT_STNE",
		"DEFAULT_PT_O2",
	};
	for (std::size_t index = 0; index < state.completedStages.size(); ++index)
	{
		if (state.completedStages[index] != stages[index].id)
		{
			return false;
		}
		for (std::size_t output = 0; output < stages[index].outputCount; ++output)
		{
			auto type = stages[index].outputs[output];
			if (type <= PT_NONE || type >= PT_NUM)
			{
				return false;
			}
			auto const &identifier = SimulationData::CRef().elements[type].Identifier;
			if (identifier.empty() || identifier.size() > OmniAlchemyMaxIdentifierBytes)
			{
				return false;
			}
			expected.insert(identifier);
		}
	}

	std::set<ByteString> imported;
	for (auto const &identifier : state.unlockedIdentifiers)
	{
		if (identifier.empty() || identifier.size() > OmniAlchemyMaxIdentifierBytes || !imported.insert(identifier).second)
		{
			return false;
		}
	}
	if (imported != expected || state.records != state.completedStages)
	{
		return false;
	}
	for (std::size_t index = 0; index < OmniAlchemyStageCount; ++index)
	{
		if (state.dwellFrames[index] < 0 || state.dwellFrames[index] > 600)
		{
			return false;
		}
		if (index < state.completedStages.size() && (state.dwellFrames[index] != 0 || state.coolingArmed[index]))
		{
			return false;
		}
		if (index > state.completedStages.size() && (state.dwellFrames[index] != 0 || state.coolingArmed[index]))
		{
			return false;
		}
	}
	return true;
}

bool OmniAlchemy::Import(OmniAlchemySaveState const &state)
{
	if (!state.present)
	{
		Reset();
		return true;
	}
	if (!ValidateImportedState(state))
	{
		Reset();
		pendingNoticeKeys.emplace_back("alchemy.progress.reset_notice");
		return false;
	}
	unlockedIdentifiers = std::set<ByteString>(state.unlockedIdentifiers.begin(), state.unlockedIdentifiers.end());
	completedStages = state.completedStages;
	records = state.records;
	dwellFrames = state.dwellFrames;
	coolingArmed = state.coolingArmed;
	pendingNoticeKeys.clear();
	ResetTransientScan();
	return true;
}

OmniAlchemySaveState OmniAlchemy::Export() const
{
	OmniAlchemySaveState state;
	state.present = true;
	state.valid = true;
	state.schemaVersion = OmniAlchemyProgressSchemaVersion;
	state.unlockedIdentifiers.assign(unlockedIdentifiers.begin(), unlockedIdentifiers.end());
	state.completedStages = completedStages;
	state.records = records;
	state.dwellFrames = dwellFrames;
	state.coolingArmed = coolingArmed;
	return state;
}

bool OmniAlchemy::IsIdentifierUnlocked(std::string_view identifier) const
{
	if (Mastered())
	{
		return true;
	}
	return unlockedIdentifiers.find(CopyView(identifier)) != unlockedIdentifiers.end();
}

bool OmniAlchemy::IsElementUnlocked(int elementId) const
{
	if (elementId == PT_NONE)
	{
		return true;
	}
	if (elementId < 0 || elementId >= PT_NUM)
	{
		return false;
	}
	auto const &identifier = SimulationData::CRef().elements[elementId].Identifier;
	return !identifier.empty() && IsIdentifierUnlocked(std::string_view(identifier.data(), identifier.size()));
}

bool OmniAlchemy::AreNear(std::array<int, 4> const &targets, std::size_t targetCount, int radius) const
{
	if (!targetCount || radius <= 0)
	{
		return false;
	}
	auto anchorType = targets[0];
	for (std::size_t anchorIndex = 0; anchorIndex < scan.positionCounts[anchorType]; ++anchorIndex)
	{
		auto const anchor = scan.positions[anchorType][anchorIndex];
		bool allNear = true;
		for (std::size_t targetIndex = 1; targetIndex < targetCount && allNear; ++targetIndex)
		{
			auto type = targets[targetIndex];
			bool found = false;
			for (std::size_t positionIndex = 0; positionIndex < scan.positionCounts[type]; ++positionIndex)
			{
				auto const position = scan.positions[type][positionIndex];
				if (std::max(std::abs(position.x - anchor.x), std::abs(position.y - anchor.y)) <= radius)
				{
					found = true;
					break;
				}
			}
			allNear = found;
		}
		if (allNear)
		{
			return true;
		}
	}
	return false;
}

bool OmniAlchemy::HasFilteredPair(int medium, int fluid, int minimumCount) const
{
	if (medium <= PT_NONE || fluid <= PT_NONE || scan.counts[medium] < minimumCount || scan.counts[fluid] < minimumCount)
	{
		return false;
	}
	std::array<int, 4> pair{ medium, fluid };
	return AreNear(pair, 2, 8);
}

void OmniAlchemy::UnlockElement(int elementId)
{
	if (elementId <= PT_NONE || elementId >= PT_NUM)
	{
		return;
	}
	auto const &identifier = SimulationData::CRef().elements[elementId].Identifier;
	if (!identifier.empty())
	{
		unlockedIdentifiers.insert(identifier);
	}
}

bool OmniAlchemy::FinishScan()
{
	auto stageIndex = completedStages.size();
	if (stageIndex >= stages.size())
	{
		return false;
	}
	auto const &stage = stages[stageIndex];
	bool conditionsPass = true;
	for (std::size_t index = 0; index < stage.inputCount; ++index)
	{
		conditionsPass = conditionsPass && scan.counts[stage.inputs[index]] > 0;
	}
	if (stage.temperatureTarget != PT_NONE)
	{
		conditionsPass = conditionsPass && scan.maximumTemperature[stage.temperatureTarget] >= stage.minimumTemperature;
	}
	if (stage.pressureTarget != PT_NONE)
	{
		conditionsPass = conditionsPass && scan.maximumAbsolutePressure[stage.pressureTarget] >= stage.minimumAbsolutePressure;
	}
	if (stage.electricTarget != PT_NONE)
	{
		conditionsPass = conditionsPass && scan.counts[stage.electricTarget] >= stage.minimumElectricCount;
	}
	if (stage.catalystTarget != PT_NONE)
	{
		conditionsPass = conditionsPass && scan.counts[stage.catalystTarget] >= stage.minimumCatalystCount;
	}
	if (stage.structureTargetCount)
	{
		conditionsPass = conditionsPass && AreNear(stage.structureTargets, stage.structureTargetCount, stage.structureRadius);
	}
	if (stage.filterMedium != PT_NONE)
	{
		conditionsPass = conditionsPass && HasFilteredPair(stage.filterMedium, stage.filterFluid, stage.filterMinimumCount);
	}
	if (stage.prerequisiteMask)
	{
		std::uint16_t completedMask = completedStages.empty()
			? 0
			: std::uint16_t((1U << completedStages.size()) - 1U);
		conditionsPass = conditionsPass && (completedMask & stage.prerequisiteMask) == stage.prerequisiteMask;
	}
	if (stage.coolingHeatTarget != PT_NONE)
	{
		auto wasArmed = coolingArmed[stageIndex];
		if (scan.maximumTemperature[stage.coolingHeatTarget] >= stage.coolingMinimumTemperature)
		{
			coolingArmed[stageIndex] = true;
		}
		conditionsPass = conditionsPass && wasArmed &&
			scan.minimumTemperature[stage.coolingCoolTarget] <= stage.coolingMaximumTemperature;
	}
	if (stage.continuousFrames)
	{
		if (conditionsPass)
		{
			dwellFrames[stageIndex] = std::min(stage.continuousFrames, dwellFrames[stageIndex] + AlchemyScanIntervalFrames);
		}
		else
		{
			dwellFrames[stageIndex] = 0;
		}
		conditionsPass = conditionsPass && dwellFrames[stageIndex] >= stage.continuousFrames;
	}
	if (!conditionsPass)
	{
		return false;
	}

	completedStages.emplace_back(stage.id);
	records.emplace_back(stage.id);
	dwellFrames[stageIndex] = 0;
	coolingArmed[stageIndex] = false;
	for (std::size_t index = 0; index < stage.outputCount; ++index)
	{
		UnlockElement(stage.outputs[index]);
	}
	pendingNoticeKeys.emplace_back(stage.noticeKey);
	if (completedStages.size() == stages.size())
	{
		pendingNoticeKeys.emplace_back("alchemy.mastery.notice");
	}
	return true;
}

bool OmniAlchemy::Observe(Simulation const &simulation)
{
	if (Mastered())
	{
		ResetTransientScan();
		return false;
	}
	if (!scan.active)
	{
		if (simulation.frameCount % AlchemyScanIntervalFrames)
		{
			return false;
		}
		BeginScan();
	}

	auto end = std::min(simulation.parts.active, scan.cursor + AlchemyScanParticleBudget);
	for (int index = scan.cursor; index < end; ++index)
	{
		auto const &particle = simulation.parts[index];
		auto type = TYP(particle.type);
		if (type <= PT_NONE || type >= PT_NUM)
		{
			continue;
		}
		scan.counts[type] += 1;
		scan.maximumTemperature[type] = std::max(scan.maximumTemperature[type], particle.temp);
		scan.minimumTemperature[type] = std::min(scan.minimumTemperature[type], particle.temp);
		auto x = std::clamp(int(particle.x + 0.5f), 0, XRES - 1);
		auto y = std::clamp(int(particle.y + 0.5f), 0, YRES - 1);
		scan.maximumAbsolutePressure[type] = std::max(
			scan.maximumAbsolutePressure[type],
			std::abs(simulation.pv[y / CELL][x / CELL])
		);
		if (scan.positionCounts[type] < PositionSamplesPerType)
		{
			scan.positions[type][scan.positionCounts[type]++] = { x, y };
		}
	}
	scan.cursor = end;
	if (scan.cursor < simulation.parts.active)
	{
		return false;
	}
	auto advanced = FinishScan();
	ResetTransientScan();
	return advanced;
}

ByteString OmniAlchemy::ConsumeNoticeKey()
{
	if (pendingNoticeKeys.empty())
	{
		return {};
	}
	auto value = std::move(pendingNoticeKeys.front());
	pendingNoticeKeys.pop_front();
	return value;
}

ByteString OmniAlchemy::CurrentStageId() const
{
	return completedStages.size() < stages.size()
		? ByteString(stages[completedStages.size()].id)
		: ByteString();
}

ByteString OmniAlchemy::CurrentHintKey() const
{
	return completedStages.size() < stages.size()
		? ByteString(stages[completedStages.size()].hintKey)
		: ByteString("alchemy.mastery.notice");
}

ByteString OmniAlchemy::StageId(std::size_t index) const
{
	return index < stages.size() ? ByteString(stages[index].id) : ByteString();
}

ByteString OmniAlchemy::StageNameKey(std::size_t index) const
{
	return index < stages.size() ? ByteString(stages[index].nameKey) : ByteString();
}

int OmniAlchemy::CurrentDwellFrames() const
{
	return completedStages.size() < dwellFrames.size() ? dwellFrames[completedStages.size()] : 0;
}

bool OmniAlchemy::CurrentCoolingArmed() const
{
	return completedStages.size() < coolingArmed.size() && coolingArmed[completedStages.size()];
}

std::size_t OmniAlchemy::CompletedStageCount() const
{
	return completedStages.size();
}

bool OmniAlchemy::Mastered() const
{
	return completedStages.size() == stages.size();
}
