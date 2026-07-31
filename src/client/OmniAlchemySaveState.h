#pragma once

#include "common/String.h"

#include <array>
#include <vector>

constexpr int OmniAlchemyProgressSchemaVersion = 1;
constexpr std::size_t OmniAlchemyStageCount = 10;
constexpr std::size_t OmniAlchemyMaxSavedIdentifiers = 512;
constexpr std::size_t OmniAlchemyMaxSavedRecords = 64;
constexpr std::size_t OmniAlchemyMaxIdentifierBytes = 64;
constexpr std::size_t OmniAlchemyMaxStageIdBytes = 32;

struct OmniAlchemySaveState
{
	bool present = false;
	bool valid = true;
	int schemaVersion = OmniAlchemyProgressSchemaVersion;
	std::vector<ByteString> unlockedIdentifiers;
	std::vector<ByteString> completedStages;
	std::vector<ByteString> records;
	std::array<int, OmniAlchemyStageCount> dwellFrames{};
	std::array<bool, OmniAlchemyStageCount> coolingArmed{};
};
