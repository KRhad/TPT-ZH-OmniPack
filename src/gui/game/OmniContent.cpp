#include "OmniContent.h"

#include "gui/elementsearch/ElementCatalog.h"
#include "gui/game/tool/Tool.h"
#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "simulation/Particle.h"
#include "simulation/ElementDefs.h"
#include "simulation/SimulationData.h"

namespace
{
constexpr std::array<OmniSettingDefinition, OmniSettingCount> settingDefinitions{ {
	{ OmniSetting::Biology,              "Omni.Modules.Biology",              "options.omni.biology",               "options.omni.biology.info",               true,  true  },
	{ OmniSetting::Metallurgy,            "Omni.Modules.Metallurgy",            "options.omni.metallurgy",             "options.omni.metallurgy.info",             true,  true  },
	{ OmniSetting::Chemistry,             "Omni.Modules.Chemistry",             "options.omni.chemistry",              "options.omni.chemistry.info",              true,  true  },
	{ OmniSetting::AdvancedNuclear,       "Omni.Modules.AdvancedNuclear",       "options.omni.advanced_nuclear",       "options.omni.advanced_nuclear.info",       true,  true  },
	{ OmniSetting::SpecialPhysics,        "Omni.Modules.SpecialPhysics",        "options.omni.special_physics",        "options.omni.special_physics.info",        true,  true  },
	{ OmniSetting::Disasters,             "Omni.Modules.Disasters",             "options.omni.disasters",              "options.omni.disasters.info",              true,  true  },
	{ OmniSetting::Experimental,          "Omni.Modules.Experimental",          "options.omni.experimental",           "options.omni.experimental.info",           false, true  },
	{ OmniSetting::SimplifiedBiology,     "Omni.Simulation.SimplifiedBiology",  "options.omni.simplified_biology",     "options.omni.simplified_biology.info",     false, true  },
	{ OmniSetting::PerformanceProtection, "Omni.Simulation.PerformanceGuard",   "options.omni.performance_protection", "options.omni.performance_protection.info", false, false },
	{ OmniSetting::DetailedHud,           "Omni.Interface.DetailedHud",         "options.omni.detailed_hud",            "options.omni.detailed_hud.info",            false, false },
	{ OmniSetting::AlchemyMode,           "Omni.Progress.AlchemyMode",          "options.omni.alchemy_mode",            "options.omni.alchemy_mode.info",            false, false },
} };

static_assert(settingDefinitions.size() == OmniSettingCount);
static_assert(OmniOfficialLastId + 1 == OmniReservedFirstId);
static_assert(OmniReservedLastId + 1 == OmniMetallurgyFirstId);
static_assert(OmniMetallurgyLastId + 1 == OmniBiologyFirstId);
static_assert(OmniBiologyLastId + 1 == OmniNuclearFirstId);
static_assert(OmniNuclearLastId + 1 == OmniChemistryFirstId);
static_assert(OmniChemistryLastId + 1 == OmniAutomationFirstId);
static_assert(OmniAutomationLastId + 1 == OmniSpecialPhysicsFirstId);
static_assert(OmniSpecialPhysicsLastId + 1 == OmniDisastersFirstId);
static_assert(OmniDisastersLastId + 1 == OmniCompatibilityFirstId);
static_assert(OmniCompatibilityLastId + 1 == OmniExperimentalFirstId);
static_assert(OmniExperimentalLastId == PT_NUM - 1);

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
	if (elementId <= OmniAutomationLastId)
	{
		return OmniElementModule::Automation;
	}
	if (elementId <= OmniSpecialPhysicsLastId)
	{
		return OmniElementModule::SpecialPhysics;
	}
	if (elementId <= OmniDisastersLastId)
	{
		return OmniElementModule::Disasters;
	}
	if (elementId <= OmniCompatibilityLastId)
	{
		return OmniElementModule::Compatibility;
	}
	return OmniElementModule::Experimental;
}

bool IsOmniElementSelectable(int elementId)
{
	if (elementId < 0 || elementId >= PT_NUM)
	{
		return false;
	}

	switch (GetOmniElementModule(elementId))
	{
	case OmniElementModule::Biology:
		return GetOmniSetting(OmniSetting::Biology);
	case OmniElementModule::Metallurgy:
		return GetOmniSetting(OmniSetting::Metallurgy);
	case OmniElementModule::Chemistry:
		return GetOmniSetting(OmniSetting::Chemistry);
	case OmniElementModule::AdvancedNuclear:
		return GetOmniSetting(OmniSetting::AdvancedNuclear);
	case OmniElementModule::SpecialPhysics:
		return GetOmniSetting(OmniSetting::SpecialPhysics);
	case OmniElementModule::Disasters:
		return GetOmniSetting(OmniSetting::Disasters);
	case OmniElementModule::Experimental:
		return GetOmniSetting(OmniSetting::Experimental);
	case OmniElementModule::Reserved:
	case OmniElementModule::Compatibility:
		return false;
	default:
		return true;
	}
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
	return IsOmniElementSelectable(elementId);
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
	case OmniElementModule::SpecialPhysics:
		return "options.omni.special_physics";
	case OmniElementModule::Disasters:
		return "options.omni.disasters";
	case OmniElementModule::Experimental:
		return "options.omni.experimental";
	default:
		return "";
	}
}

std::vector<OmniElementModule> FindDisabledOmniSaveModules(GameSave const &save)
{
	constexpr std::size_t moduleCount = static_cast<std::size_t>(OmniElementModule::Experimental) + 1;
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
		if (!record || !record->identifier.starts_with("OMNI_PT_") || record->implementationStatus != "implemented" || IsOmniElementSelectable(type))
		{
			return;
		}
		found[static_cast<std::size_t>(GetOmniElementModule(type))] = true;
	};

	for (int index = 0; index < NPART && index < save.particlesCount && index < static_cast<int>(save.particles.size()); ++index)
	{
		auto const &particle = save.particles[index];
		auto type = TYP(particle.type);
		if (type <= 0 || type >= PT_NUM)
		{
			continue;
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
