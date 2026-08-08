#include "OmniContent.h"

#include "gui/elementsearch/ElementCatalog.h"
#include "gui/game/tool/Tool.h"
#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/Particle.h"
#include "simulation/ElementDefs.h"
#include "simulation/SimulationData.h"
#include "simulation/OmniMetallurgy.h"

namespace
{
constexpr std::array<OmniSettingDefinition, OmniSettingCount> settingDefinitions{ {
	{ OmniSetting::Biology,              "Omni.Modules.Biology",              "options.omni.biology",               "options.omni.biology.info",               true,  true  },
	{ OmniSetting::Metallurgy,            "Omni.Modules.Metallurgy",            "options.omni.metallurgy",             "options.omni.metallurgy.info",             true,  true  },
	{ OmniSetting::Chemistry,             "Omni.Modules.Chemistry",             "options.omni.chemistry",              "options.omni.chemistry.info",              true,  true  },
	{ OmniSetting::AdvancedNuclear,       "Omni.Modules.AdvancedNuclear",       "options.omni.advanced_nuclear",       "options.omni.advanced_nuclear.info",       true,  true  },
	{ OmniSetting::Electronics,           "Omni.Modules.Electronics",           "options.omni.electronics",            "options.omni.electronics.info",            true,  true  },
	{ OmniSetting::SimplifiedBiology,     "Omni.Simulation.SimplifiedBiology",  "options.omni.simplified_biology",     "options.omni.simplified_biology.info",     false, true  },
} };

static_assert(settingDefinitions.size() == OmniSettingCount);
static_assert(OmniOfficialLastId + 1 == OmniReservedFirstId);
static_assert(OmniReservedLastId + 1 == OmniMetallurgyFirstId);
static_assert(OmniMetallurgyLastId + 1 == OmniBiologyFirstId);
static_assert(OmniBiologyLastId + 1 == OmniNuclearFirstId);
static_assert(OmniNuclearLastId + 1 == OmniChemistryFirstId);
static_assert(OmniChemistryLastId + 1 == OmniPeriodicFirstId);
static_assert(OmniPeriodicLastId + 1 == OmniChemistryExpansionFirstId);
static_assert(OmniChemistryExpansionLastId + 1 == OmniEngineeringFirstId);
static_assert(OmniEngineeringLastId + 1 == OmniIsotopeFirstId);
static_assert(OmniIsotopeLastId + 1 == OmniOrganicFirstId);
static_assert(OmniOrganicLastId + 1 == OmniElectronicsFirstId);
static_assert(OmniElectronicsLastId + 1 == OmniEnvironmentFirstId);
static_assert(OmniEnvironmentLastId + 1 == OmniFutureContentFirstId);
static_assert(OmniFutureContentLastId == PT_NUM - 1);

OmniSettingDefinition const *DefinitionFor(OmniSetting setting)
{
	auto index = static_cast<std::size_t>(setting);
	if (index >= settingDefinitions.size())
	{
		return nullptr;
	}
	return &settingDefinitions[index];
}
}

std::array<OmniSettingDefinition, OmniSettingCount> const &GetOmniSettingDefinitions()
{
	return settingDefinitions;
}

bool GetOmniSetting(OmniSetting setting)
{
	auto const *definition = DefinitionFor(setting);
	return definition && definition->available
		? GlobalPrefs::Ref().Get(definition->preferenceKey, definition->defaultEnabled)
		: false;
}

void SetOmniSetting(OmniSetting setting, bool enabled)
{
	if (auto const *definition = DefinitionFor(setting); definition && definition->available)
	{
		GlobalPrefs::Ref().Set(definition->preferenceKey, enabled);
	}
}

OmniElementModule GetOmniElementModule(int elementId)
{
	if (elementId <= OmniOfficialLastId)
	{
		return OmniElementModule::Official;
	}
	if (elementId <= OmniReservedLastId)
	{
		return OmniElementModule::Reserved;
	}
	if (elementId <= OmniMetallurgyLastId)
	{
		return OmniElementModule::Metallurgy;
	}
	if (elementId <= OmniBiologyLastId)
	{
		return OmniElementModule::Biology;
	}
	if (elementId <= OmniNuclearLastId)
	{
		return OmniElementModule::AdvancedNuclear;
	}
	if (elementId <= OmniChemistryLastId)
	{
		return OmniElementModule::Chemistry;
	}
	if (elementId <= OmniPeriodicLastId)
	{
		return OmniElementModule::Periodic;
	}
	if (elementId <= OmniChemistryExpansionLastId)
	{
		return OmniElementModule::Chemistry;
	}
	if (elementId <= OmniEngineeringLastId)
	{
		return OmniElementModule::Metallurgy;
	}
	if (elementId <= OmniIsotopeLastId)
	{
		return OmniElementModule::AdvancedNuclear;
	}
	if (elementId <= OmniOrganicLastId)
	{
		return OmniElementModule::Chemistry;
	}
	if (elementId <= OmniElectronicsLastId)
	{
		return OmniElementModule::Electronics;
	}
	if (elementId <= OmniEnvironmentLastId)
	{
		return OmniElementModule::Biology;
	}
	return OmniElementModule::FutureContent;
}

OmniSelectionRestriction GetOmniElementSelectionRestriction(int elementId)
{
	if (elementId < 0 || elementId >= PT_NUM)
	{
		return OmniSelectionRestriction::InvalidElement;
	}
	if (auto const *record = FindElementCatalogByStableId(elementId);
		record && !record->duplicateOf.empty())
	{
		return OmniSelectionRestriction::CompatibilityAlias;
	}

	switch (GetOmniElementModule(elementId))
	{
	case OmniElementModule::Biology:
		if (!GetOmniSetting(OmniSetting::Biology)) return OmniSelectionRestriction::ModuleDisabled;
		break;
	case OmniElementModule::Metallurgy:
		if (!GetOmniSetting(OmniSetting::Metallurgy)) return OmniSelectionRestriction::ModuleDisabled;
		break;
	case OmniElementModule::Chemistry:
		if (!GetOmniSetting(OmniSetting::Chemistry)) return OmniSelectionRestriction::ModuleDisabled;
		break;
	case OmniElementModule::AdvancedNuclear:
		if (!GetOmniSetting(OmniSetting::AdvancedNuclear)) return OmniSelectionRestriction::ModuleDisabled;
		break;
	case OmniElementModule::Electronics:
		if (!GetOmniSetting(OmniSetting::Electronics)) return OmniSelectionRestriction::ModuleDisabled;
		break;
	case OmniElementModule::Reserved:
	case OmniElementModule::FutureContent:
		return OmniSelectionRestriction::ReservedElement;
	default:
		break;
	}
	return OmniSelectionRestriction::None;
}

bool IsOmniElementSelectable(int elementId)
{
	return GetOmniElementSelectionRestriction(elementId) == OmniSelectionRestriction::None;
}

bool IsOmniElementCreationAllowed(int elementId)
{
	if (elementId < 0 || elementId >= PT_NUM)
	{
		return false;
	}
	auto const *record = FindElementCatalogByStableId(elementId);
	if (!record)
	{
		return true;
	}
	auto const &runtimeIdentifier = SimulationData::CRef().elements[elementId].Identifier;
	if (record->identifier != std::string_view(runtimeIdentifier.data(), runtimeIdentifier.size()))
	{
		return true;
	}
	return IsOmniElementSelectable(elementId);
}

bool IsOmniToolSelectable(Tool const &tool)
{
	if (!tool.IsElement)
	{
		return true;
	}

	auto elementId = TYP(tool.ToolID);
	auto const *record = FindElementCatalogByStableId(elementId);
	auto identifier = std::string_view(tool.Identifier.data(), tool.Identifier.size());
	if (!record || record->identifier != identifier)
	{
		// Lua elements and custom GOL tools are runtime content, not an
		// integrated module. Numeric IDs alone must never classify them.
		return true;
	}
	return GetOmniElementSelectionRestriction(elementId) == OmniSelectionRestriction::None;
}

char const *GetOmniElementModuleNameKey(OmniElementModule module)
{
	switch (module)
	{
	case OmniElementModule::Biology:
		return "options.omni.biology";
	case OmniElementModule::Metallurgy:
		return "options.omni.metallurgy";
	case OmniElementModule::Chemistry:
		return "options.omni.chemistry";
	case OmniElementModule::AdvancedNuclear:
		return "options.omni.advanced_nuclear";
	case OmniElementModule::Electronics:
		return "options.omni.electronics";
	case OmniElementModule::Periodic:
		return "periodic.table.title";
	default:
		return "";
	}
}

std::vector<OmniElementModule> FindDisabledOmniSaveModules(GameSave const &save)
{
	constexpr std::size_t moduleCount = static_cast<std::size_t>(OmniElementModule::FutureContent) + 1;
	std::array<bool, moduleCount> found{};
	auto const &elements = SimulationData::CRef().elements;
	auto const &possiblyCarriesType = Particle::PossiblyCarriesType();
	auto const &properties = Particle::GetProperties();

	auto inspectType = [&found](int type) {
		type = TYP(type);
		if (type <= 0 || type >= PT_NUM)
		{
			return;
		}
		auto const *record = FindElementCatalogByStableId(type);
		if (!record || !record->identifier.starts_with("OMNI_PT_") || record->implementationStatus != "implemented")
		{
			return;
		}
		auto module = GetOmniElementModule(type);
		auto restriction = GetOmniElementSelectionRestriction(type);
		if (restriction == OmniSelectionRestriction::ModuleDisabled
			|| (restriction == OmniSelectionRestriction::CompatibilityAlias
				&& module == OmniElementModule::Metallurgy
				&& !GetOmniSetting(OmniSetting::Metallurgy)))
		{
			found[static_cast<std::size_t>(module)] = true;
		}
	};

	for (int index = 0; index < NPART && index < save.particlesCount && index < static_cast<int>(save.particles.size()); ++index)
	{
		auto const &particle = save.particles[index];
		auto type = TYP(particle.type);
		if (type <= 0 || type >= PT_NUM)
		{
			continue;
		}
		if (IsOmniRecoverableScrap(particle)
			&& !GetOmniSetting(OmniSetting::Metallurgy))
		{
			found[static_cast<std::size_t>(OmniElementModule::Metallurgy)] = true;
		}

		inspectType(type);
		for (auto propertyIndex : possiblyCarriesType)
		{
			if (!(elements[type].CarriesTypeIn & (1U << propertyIndex)))
			{
				continue;
			}
			auto const *property = reinterpret_cast<int const *>(reinterpret_cast<char const *>(&particle) + properties[propertyIndex].Offset);
			inspectType(TYP(*property));
		}
	}

	std::vector<OmniElementModule> modules;
	for (std::size_t index = 0; index < found.size(); ++index)
	{
		if (found[index])
		{
			modules.push_back(static_cast<OmniElementModule>(index));
		}
	}
	return modules;
}
