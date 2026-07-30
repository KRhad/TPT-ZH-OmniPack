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
	SpecialPhysics,
	Disasters,
	Experimental,
	SimplifiedBiology,
	PerformanceProtection,
	DetailedHud,
	AlchemyMode,
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
	Automation,
	SpecialPhysics,
	Disasters,
	Compatibility,
	Experimental,
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
constexpr int OmniChemistryLastId = 391;
constexpr int OmniAutomationFirstId = 392;
constexpr int OmniAutomationLastId = 423;
constexpr int OmniSpecialPhysicsFirstId = 424;
constexpr int OmniSpecialPhysicsLastId = 439;
constexpr int OmniDisastersFirstId = 440;
constexpr int OmniDisastersLastId = 455;
constexpr int OmniCompatibilityFirstId = 456;
constexpr int OmniCompatibilityLastId = 479;
constexpr int OmniExperimentalFirstId = 480;
constexpr int OmniExperimentalLastId = 511;

std::array<OmniSettingDefinition, OmniSettingCount> const &GetOmniSettingDefinitions();
bool GetOmniSetting(OmniSetting setting);
void SetOmniSetting(OmniSetting setting, bool enabled);

OmniElementModule GetOmniElementModule(int elementId);
bool IsOmniElementSelectable(int elementId);
bool IsOmniToolSelectable(Tool const &tool);
std::vector<OmniElementModule> FindDisabledOmniSaveModules(GameSave const &save);
char const *GetOmniElementModuleNameKey(OmniElementModule module);
