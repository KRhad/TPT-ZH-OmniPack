#pragma once

#include <array>
#include <cstddef>
#include <vector>

class Tool;
class GameSave;

enum class OmniSetting : unsigned char
{
	Biology,
	Metallurgy,
	Chemistry,
	AdvancedNuclear,
	SimplifiedBiology,
	Count,
};

enum class OmniElementModule : unsigned char
{
	Official,
	Reserved,
	Metallurgy,
	Biology,
	AdvancedNuclear,
	Chemistry,
	Periodic,
	FutureContent,
};

enum class OmniSelectionRestriction : unsigned char
{
	None,
	InvalidElement,
	ReservedElement,
	ModuleDisabled,
};

struct OmniSettingDefinition
{
	OmniSetting setting;
	char const *preferenceKey;
	char const *labelKey;
	char const *infoKey;
	bool defaultEnabled;
	bool available;
};

constexpr std::size_t OmniSettingCount = static_cast<std::size_t>(OmniSetting::Count);

// Stable custom element allocation. These ranges are part of the save compatibility
// contract and must not be shifted when modules gain or lose elements.
constexpr int OmniOfficialLastId = 195;
constexpr int OmniReservedFirstId = 196;
constexpr int OmniReservedLastId = 255;
constexpr int OmniMetallurgyFirstId = 256;
constexpr int OmniMetallurgyLastId = 287;
constexpr int OmniBiologyFirstId = 288;
constexpr int OmniBiologyLastId = 327;
constexpr int OmniNuclearFirstId = 328;
constexpr int OmniNuclearLastId = 359;
constexpr int OmniChemistryFirstId = 360;
constexpr int OmniChemistryLastId = 369;
constexpr int OmniPeriodicFirstId = 370;
constexpr int OmniPeriodicLastId = 461;
constexpr int OmniChemistryExpansionFirstId = 462;
constexpr int OmniChemistryExpansionLastId = 477;
constexpr int OmniFutureContentFirstId = 478;
constexpr int OmniFutureContentLastId = 511;

std::array<OmniSettingDefinition, OmniSettingCount> const &GetOmniSettingDefinitions();
bool GetOmniSetting(OmniSetting setting);
void SetOmniSetting(OmniSetting setting, bool enabled);

OmniElementModule GetOmniElementModule(int elementId);
OmniSelectionRestriction GetOmniElementSelectionRestriction(int elementId);
bool IsOmniElementSelectable(int elementId);
bool IsOmniElementCreationAllowed(int elementId);
bool IsOmniToolSelectable(Tool const &tool);
std::vector<OmniElementModule> FindDisabledOmniSaveModules(GameSave const &save);
char const *GetOmniElementModuleNameKey(OmniElementModule module);
