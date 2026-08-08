#pragma once

#include <optional>
#include <span>
#include <string_view>

class Tool;

enum class OmniContentKind
{
	PeriodicElement,
	Isotope,
	InorganicCompound,
	OrganicMaterial,
	AlloyEngineering,
	EcologyMaterial,
	NuclearDevice,
	CustomSpecial,
	Official,
	CompatibilityAlias,
	HiddenInternal,
};

enum class OmniMainMenuPolicy
{
	OfficialDefault,
	PeriodicOnly,
	OrganicMenu,
	AlloyMenu,
	ExistingDedicatedMenu,
	PreserveExisting,
	Hidden,
};

struct OmniContentPresentationRecord
{
	std::string_view identifier;
	int stableId;
	OmniContentKind contentKind;
	OmniMainMenuPolicy mainMenuPolicy;
	std::string_view periodicAtomicNumbers;
	std::string_view standaloneMenu;
};

std::span<OmniContentPresentationRecord const> GetOmniContentPresentationData();
OmniContentPresentationRecord const *FindOmniContentPresentation(int stableId);
std::optional<int> GetOmniStandardMenuSection(Tool const &tool);
