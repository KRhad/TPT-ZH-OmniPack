#include "client/GameSave.h"
#include "simulation/OmniAtmosphere.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"
#include "simulation/SnapshotDelta.h"
#include "simulation/ElementClasses.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
int Fail(std::string_view message)
{
	std::cerr << "omni-save-roundtrip-probe: FAIL " << message << std::endl;
	return 1;
}

bool SameCell(const OmniAtmosphere &left, const OmniAtmosphere &right, std::size_t x, std::size_t y)
{
	const auto &leftState = left.State(x, y);
	const auto &rightState = right.State(x, y);
	if (leftState.density != rightState.density ||
		leftState.momentumX != rightState.momentumX ||
		leftState.momentumY != rightState.momentumY ||
		leftState.totalEnergy != rightState.totalEnergy ||
		left.Primitive(x, y).condensedWaterDensity != right.Primitive(x, y).condensedWaterDensity)
	{
		return false;
	}
	for (std::size_t species = 0; species < left.SpeciesCount(); ++species)
	{
		if (left.SpeciesMassDensity(x, y, species) != right.SpeciesMassDensity(x, y, species))
			return false;
	}
	return true;
}

bool SameAtmosphere(const OmniAtmosphere &left, const OmniAtmosphere &right)
{
	if (left.Width() != right.Width() || left.Height() != right.Height() ||
		left.SpeciesCount() != right.SpeciesCount())
	{
		return false;
	}
	for (std::size_t y = 0; y < left.Height(); ++y)
	{
		for (std::size_t x = 0; x < left.Width(); ++x)
		{
			if (!SameCell(left, right, x, y))
				return false;
		}
	}
	return true;
}

void ConfigurePayload(GameSave &save)
{
	const auto species = OmniDefaultAtmosphereSpecies();
	const std::size_t cellCount = std::size_t(save.blockSize.X) * std::size_t(save.blockSize.Y);
	save.hasOmniAtmosphereState = true;
	save.omniAtmosphereStateVersion = GameSave::OmniAtmosphereStateVersion;
	for (const auto &definition : species)
		save.omniAtmosphereSpecies.emplace_back(definition.id);
	save.omniAtmosphereSpeciesMassDensity.resize(cellCount * species.size(), 0.0);
	save.omniAtmosphereMomentumX.resize(cellCount, 0.0);
	save.omniAtmosphereMomentumY.resize(cellCount, 0.0);
	save.omniAtmosphereTotalEnergy.resize(cellCount, 250000.0);
	save.omniAtmosphereCondensedWaterDensity.resize(cellCount, 0.0);
	save.omniAtmosphereCellValid.assign(cellCount, 1);
	for (std::size_t cell = 0; cell < cellCount; ++cell)
	{
		for (std::size_t channel = 0; channel < species.size(); ++channel)
			save.omniAtmosphereSpeciesMassDensity[cell * species.size() + channel] =
				0.1 + double(cell) * 0.01 + double(channel) * 0.001;
		save.omniAtmosphereMomentumX[cell] = 10.0 + double(cell);
		save.omniAtmosphereMomentumY[cell] = -20.0 - double(cell);
		save.omniAtmosphereTotalEnergy[cell] += double(cell) * 100.0;
		save.omniAtmosphereCondensedWaterDensity[cell] = double(cell) * 1.0e-5;
	}
}

void AdvanceOneTick(Simulation &simulation)
{
	simulation.BeforeSim(true);
	simulation.UpdateParticles(0, simulation.parts.active);
	simulation.AfterSim();
}
}

int main()
{
	SimulationData simulationData;
	auto source = Simulation::Factory();
	source->SetOmniSimulationMode(OMNI_ENHANCED);

	const std::vector<double> speciesA{ 0.61, 0.24, 0.02, 0.08, 0.05 };
	const std::vector<double> speciesB{ 0.12, 0.64, 0.01, 0.18, 0.05 };
	if (!source->omniAtmosphere->RestoreSerializedCell(5, 7, speciesA, 0.125, -0.25, 315000.0, 0.0015) ||
		!source->omniAtmosphere->RestoreSerializedCell(6, 7, speciesB, -0.5, 0.75, 420000.0, 0.0025))
	{
		return Fail("could not create a valid multi-species source state");
	}

	auto save = source->Save(true, RES.OriginRect());
	if (!save || !save->hasOmniAtmosphereState ||
		save->omniAtmosphereStateVersion != GameSave::OmniAtmosphereStateVersion)
	{
		return Fail("Enhanced full save omitted the versioned atmosphere payload");
	}
	auto serialised = save->Serialise().second;
	if (serialised.empty())
		return Fail("serialising the Enhanced atmosphere payload failed");

	GameSave parsed(serialised);
	if (!parsed.hasOmniAtmosphereState || parsed.omniAtmosphereStateVersion != GameSave::OmniAtmosphereStateVersion ||
		parsed.omniAtmosphereSpecies != save->omniAtmosphereSpecies ||
		parsed.omniAtmosphereSpeciesMassDensity != save->omniAtmosphereSpeciesMassDensity ||
		parsed.omniAtmosphereMomentumX != save->omniAtmosphereMomentumX ||
		parsed.omniAtmosphereMomentumY != save->omniAtmosphereMomentumY ||
		parsed.omniAtmosphereTotalEnergy != save->omniAtmosphereTotalEnergy ||
		parsed.omniAtmosphereCondensedWaterDensity != save->omniAtmosphereCondensedWaterDensity ||
		parsed.omniAtmosphereCellValid != save->omniAtmosphereCellValid)
	{
		return Fail("OPS parse did not preserve the exact f64 atmosphere payload");
	}

	auto loadedA = Simulation::Factory();
	auto loadedB = Simulation::Factory();
	loadedA->SetOmniSimulationMode(parsed.omniSimulationMode);
	loadedB->SetOmniSimulationMode(parsed.omniSimulationMode);
	loadedA->Load(&parsed, true, { 0, 0 });
	loadedB->Load(&parsed, true, { 0, 0 });
	if (std::string_view(loadedA->GetOmniAtmospherePersistenceStatus()) != "loaded_v2" ||
		!SameCell(*source->omniAtmosphere, *loadedA->omniAtmosphere, 5, 7) ||
		!SameAtmosphere(*loadedA->omniAtmosphere, *loadedB->omniAtmosphere))
	{
		return Fail("Simulation load did not restore the exact atmosphere state");
	}
	AdvanceOneTick(*loadedA);
	AdvanceOneTick(*loadedB);
	if (!SameAtmosphere(*loadedA->omniAtmosphere, *loadedB->omniAtmosphere))
		return Fail("save/load continuation was not deterministic");

	auto undoSource = Simulation::Factory();
	undoSource->SetOmniSimulationMode(OMNI_ENHANCED);
	if (!undoSource->omniAtmosphere->RestoreSerializedCell(
		5, 7, speciesA, 0.125, -0.25, 315000.0, 0.0015))
	{
		return Fail("could not create the undo atmosphere fixture");
	}
	undoSource->omniAtmosphere->FinalizeStateRestore();
	const int undoWater = undoSource->create_part(-1, 50, 50, PT_WATR);
	if (undoWater < 0)
		return Fail("could not create the undo water fixture");
	const double undoWaterMass = undoSource->GetOmniWaterParcelMassKg(undoWater);
	const auto oldSnapshot = undoSource->CreateSnapshot();
	undoSource->omniAtmosphere->AddSpeciesMassDensity(5, 7, OMNI_SPECIES_H2O, 0.125);
	undoSource->omniAtmosphere->AddEnergyDensity(5, 7, 10000.0);
	undoSource->parts[undoWater].temp = 450.0f;
	undoSource->kill_part(undoWater);
	const auto newSnapshot = undoSource->CreateSnapshot();
	const auto delta = SnapshotDelta::FromSnapshots(*oldSnapshot, *newSnapshot);
	const auto forwardedSnapshot = delta->Forward(*oldSnapshot);
	const auto restoredSnapshot = delta->Restore(*newSnapshot);
	if (forwardedSnapshot->Hash() != newSnapshot->Hash() ||
		restoredSnapshot->Hash() != oldSnapshot->Hash())
	{
		return Fail("SnapshotDelta did not preserve Omni atmosphere/water state");
	}
	undoSource->Restore(*oldSnapshot);
	if (undoSource->GetOmniSimulationMode() != OMNI_ENHANCED ||
		undoSource->parts[undoWater].type != PT_WATR ||
		undoSource->GetOmniWaterParcelMassKg(undoWater) != undoWaterMass)
	{
		return Fail("undo restore did not recover Enhanced water ownership state");
	}
	const std::size_t undoCell = 7 * undoSource->omniAtmosphere->Width() + 5;
	for (std::size_t species = 0; species < undoSource->omniAtmosphere->SpeciesCount(); ++species)
	{
		if (undoSource->omniAtmosphere->SpeciesMassDensity(5, 7, species) !=
			oldSnapshot->OmniAtmosphereSpeciesMassDensity[
				undoCell * undoSource->omniAtmosphere->SpeciesCount() + species])
		{
			return Fail("undo restore changed an authoritative species channel");
		}
	}
	const auto &undoState = undoSource->omniAtmosphere->State(5, 7);
	if (undoState.momentumX != oldSnapshot->OmniAtmosphereMomentumX[undoCell] ||
		undoState.momentumY != oldSnapshot->OmniAtmosphereMomentumY[undoCell] ||
		undoState.totalEnergy != oldSnapshot->OmniAtmosphereTotalEnergy[undoCell] ||
		undoSource->omniAtmosphere->Primitive(5, 7).condensedWaterDensity !=
			oldSnapshot->OmniAtmosphereCondensedWaterDensity[undoCell])
	{
		return Fail("undo restore changed authoritative momentum/energy/condensed water");
	}

	auto classic = Simulation::Factory();
	const auto classicHashBefore = classic->CreateSnapshot()->Hash();
	classic->omniAtmosphere->AddEnergyDensity(5, 7, 1000.0);
	const auto classicHashAfter = classic->CreateSnapshot()->Hash();
	if (classicHashBefore != classicHashAfter)
		return Fail("Classic snapshot hash was changed by hidden Omni state");
	auto classicSave = classic->Save(true, RES.OriginRect());
	if (!classicSave || classicSave->hasOmniAtmosphereState)
		return Fail("Classic save unexpectedly contained an OmniAtmosphere payload");
	GameSave parsedClassic(classicSave->Serialise().second);
	if (parsedClassic.hasOmniAtmosphereState)
		return Fail("Classic OPS unexpectedly parsed an OmniAtmosphere payload");

	auto noPressureSave = source->Save(false, RectSized(Vec2<int>{ 0, 0 }, Vec2<int>{ CELL * 2, CELL * 2 }));
	if (!noPressureSave || noPressureSave->hasOmniAtmosphereState)
		return Fail("includePressure=false unexpectedly saved atmosphere state");
	auto omittedTarget = Simulation::Factory();
	omittedTarget->SetOmniSimulationMode(OMNI_ENHANCED);
	const auto omittedBefore = omittedTarget->omniAtmosphere->State(12, 9);
	omittedTarget->Load(noPressureSave.get(), false, { 12, 9 });
	const auto omittedAfter = omittedTarget->omniAtmosphere->State(12, 9);
	if (omittedBefore.density != omittedAfter.density ||
		omittedBefore.momentumX != omittedAfter.momentumX ||
		omittedBefore.momentumY != omittedAfter.momentumY ||
		omittedBefore.totalEnergy != omittedAfter.totalEnergy ||
		std::string_view(omittedTarget->GetOmniAtmospherePersistenceStatus()) != "region_state_omitted")
	{
		return Fail("includePressure=false modified target atmosphere state");
	}

	GameSave legacy(Vec2<int>{ 1, 1 });
	legacy.omniSimulationMode = OMNI_ENHANCED;
	legacy.hasPressure = true;
	legacy.hasAmbientHeat = true;
	legacy.pressure[{ 0, 0 }] = 2.5f;
	legacy.velocityX[{ 0, 0 }] = 0.25f;
	legacy.velocityY[{ 0, 0 }] = -0.5f;
	legacy.ambientHeat[{ 0, 0 }] = 333.0f;
	auto migrated = Simulation::Factory();
	migrated->SetOmniSimulationMode(OMNI_ENHANCED);
	migrated->Load(&legacy, true, { 3, 4 });
	if (std::string_view(migrated->GetOmniAtmospherePersistenceStatus()) !=
		"migrated_1_0_6_legacy_projection")
	{
		return Fail("payload-free Enhanced save did not report degraded migration");
	}

	GameSave transformed(Vec2<int>{ 2, 3 });
	transformed.omniSimulationMode = OMNI_ENHANCED;
	ConfigurePayload(transformed);
	const auto oldSpecies = transformed.omniAtmosphereSpeciesMassDensity;
	const auto oldMomentumX = transformed.omniAtmosphereMomentumX;
	const auto oldMomentumY = transformed.omniAtmosphereMomentumY;
	transformed.Transform(Mat2<int>::CCW, { 0, 0 });
	const std::size_t rotatedCell = 3;
	if (transformed.blockSize != Vec2<int>{ 3, 2 } ||
		transformed.omniAtmosphereSpeciesMassDensity[rotatedCell * OMNI_COMMON_SPECIES_COUNT] != oldSpecies[0] ||
		transformed.omniAtmosphereMomentumX[rotatedCell] != oldMomentumY[0] ||
		transformed.omniAtmosphereMomentumY[rotatedCell] != -oldMomentumX[0])
	{
		return Fail("region transform did not rotate atmosphere state and momentum");
	}

	GameSave unknown(Vec2<int>{ 1, 1 });
	unknown.omniSimulationMode = OMNI_ENHANCED;
	ConfigurePayload(unknown);
	unknown.omniAtmosphereSpecies[0] = "UNKNOWN";
	auto unknownTarget = Simulation::Factory();
	unknownTarget->SetOmniSimulationMode(OMNI_ENHANCED);
	bool rejectedUnknown = false;
	try
	{
		unknownTarget->Load(&unknown, true, { 0, 0 });
	}
	catch (const std::runtime_error &)
	{
		rejectedUnknown = true;
	}
	if (!rejectedUnknown)
		return Fail("unknown species registry was not rejected");

	GameSave malformed(Vec2<int>{ 1, 1 });
	malformed.omniSimulationMode = OMNI_ENHANCED;
	ConfigurePayload(malformed);
	malformed.omniAtmosphereTotalEnergy.clear();
	auto malformedTarget = Simulation::Factory();
	malformedTarget->SetOmniSimulationMode(OMNI_ENHANCED);
	bool rejectedMalformed = false;
	try
	{
		malformedTarget->Load(&malformed, true, { 0, 0 });
	}
	catch (const std::runtime_error &)
	{
		rejectedMalformed = true;
	}
	if (!rejectedMalformed)
		return Fail("malformed in-memory payload was not rejected");

	GameSave corrupt = *save;
	corrupt.omniAtmosphereSpeciesMassDensity[0] = -1.0;
	auto corruptOps = corrupt.Serialise().second;
	if (corruptOps.empty())
		return Fail("could not construct the corrupted OPS fixture");
	bool rejectedCorrupt = false;
	try
	{
		GameSave rejected(corruptOps);
	}
	catch (const ParseException &)
	{
		rejectedCorrupt = true;
	}
	if (!rejectedCorrupt)
		return Fail("corrupted serialized species density was not rejected");

	std::cout << "omni_save_roundtrip_probe_pass=true\n";
	std::cout << "ops_state_version=" << GameSave::OmniAtmosphereStateVersion << '\n';
	std::cout << "species_channels=" << parsed.omniAtmosphereSpecies.size() << '\n';
	std::cout << "exact_f64_roundtrip=true\n";
	std::cout << "deterministic_continuation=true\n";
	std::cout << "snapshot_undo_roundtrip=true\n";
	std::cout << "snapshot_delta_roundtrip=true\n";
	std::cout << "classic_snapshot_hash_compatible=true\n";
	std::cout << "classic_payload_omitted=true\n";
	std::cout << "region_state_omitted=true\n";
	std::cout << "legacy_projection_migration=true\n";
	std::cout << "transform_momentum=true\n";
	std::cout << "unknown_species_rejected=true\n";
	std::cout << "malformed_payload_rejected=true\n";
	std::cout << "corrupt_ops_rejected=true\n";
	return 0;
}
