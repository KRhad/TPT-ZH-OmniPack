#pragma once

#include "client/OmniAlchemySaveState.h"
#include "simulation/ElementDefs.h"

#include <array>
#include <cstddef>
#include <deque>
#include <set>
#include <string_view>

class Simulation;

class OmniAlchemy
{
public:
	static OmniAlchemy &Ref();

	void Reset();
	bool Import(OmniAlchemySaveState const &state);
	OmniAlchemySaveState Export() const;

	bool IsIdentifierUnlocked(std::string_view identifier) const;
	bool IsElementUnlocked(int elementId) const;
	bool Observe(Simulation const &simulation);
	ByteString ConsumeNoticeKey();
	ByteString CurrentStageId() const;
	ByteString CurrentHintKey() const;
	ByteString StageId(std::size_t index) const;
	ByteString StageNameKey(std::size_t index) const;
	int CurrentDwellFrames() const;
	bool CurrentCoolingArmed() const;

	std::size_t CompletedStageCount() const;
	bool Mastered() const;

private:
	static constexpr std::size_t PositionSamplesPerType = 8;

	struct Position
	{
		int x = 0;
		int y = 0;
	};

	struct ScanState
	{
		bool active = false;
		int cursor = 0;
		std::array<int, PT_NUM> counts{};
		std::array<float, PT_NUM> maximumTemperature{};
		std::array<float, PT_NUM> minimumTemperature{};
		std::array<float, PT_NUM> maximumAbsolutePressure{};
		std::array<std::array<Position, PositionSamplesPerType>, PT_NUM> positions{};
		std::array<std::size_t, PT_NUM> positionCounts{};
	};

	OmniAlchemy();
	void ResetTransientScan();
	void BeginScan();
	bool FinishScan();
	bool ValidateImportedState(OmniAlchemySaveState const &state) const;
	bool AreNear(std::array<int, 4> const &targets, std::size_t targetCount, int radius) const;
	bool HasFilteredPair(int medium, int fluid, int minimumCount) const;
	void UnlockElement(int elementId);

	std::set<ByteString> unlockedIdentifiers;
	std::vector<ByteString> completedStages;
	std::vector<ByteString> records;
	std::array<int, OmniAlchemyStageCount> dwellFrames{};
	std::array<bool, OmniAlchemyStageCount> coolingArmed{};
	std::deque<ByteString> pendingNoticeKeys;
	ScanState scan;
};
