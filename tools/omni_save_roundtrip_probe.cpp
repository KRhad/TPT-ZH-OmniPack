#include "bzip2/bz2wrap.h"
#include "client/GameSave.h"
#include "common/Bson.h"
#include "simulation/OmniAtmosphere.h"
#include "simulation/OmniThermal.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"
#include "simulation/SnapshotDelta.h"
#include "simulation/ElementClasses.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
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

Bson ProbeOpsNonconformance()
{
	Bson nonconformance(Bson::Type::objectValue);
	auto &palette = (nonconformance["palette"] = Bson::Type::arrayValue);
	palette.Append("objectEncodedAsArray");
	return nonconformance;
}

std::vector<char> DecompressOpsPayload(const std::vector<char> &ops)
{
	if (ops.size() <= 12 || std::string_view(ops.data(), 4) != "OPS1")
		return {};
	const auto payloadSize = static_cast<std::uint32_t>(static_cast<unsigned char>(ops[8])) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[9])) << 8U) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[10])) << 16U) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[11])) << 24U);
	std::vector<char> payload;
	if (BZ2WDecompress(payload, std::span(ops.data() + 12, ops.size() - 12), payloadSize) !=
		BZ2WDecompressOk)
	{
		return {};
	}
	return payload;
}

template<class Mutator>
std::vector<char> MutateOmniNodeOps(const std::vector<char> &ops, const char *nodeName,
	Mutator mutate)
{
	auto payload = DecompressOpsPayload(ops);
	if (payload.empty())
		return {};
	try
	{
		auto nonconformance = ProbeOpsNonconformance();
		auto bson = Bson::Parse(payload, &nonconformance);
		auto *omniNode = bson.Get(nodeName);
		if (!omniNode || !omniNode->Is<Bson::Object>())
			return {};
		mutate(bson[nodeName]);
		payload = bson.Dump(&nonconformance);
	}
	catch (const std::exception &)
	{
		return {};
	}

	std::vector<char> compressed;
	if (BZ2WCompress(compressed, payload) != BZ2WCompressOk)
		return {};
	std::vector<char> mutated(ops.begin(), ops.begin() + 12);
	const auto payloadSize = static_cast<std::uint32_t>(payload.size());
	for (std::size_t byte = 0; byte < sizeof(payloadSize); ++byte)
		mutated[8 + byte] = static_cast<char>((payloadSize >> (byte * 8U)) & 0xFFU);
	mutated.insert(mutated.end(), compressed.begin(), compressed.end());
	return mutated;
}

template<class Mutator>
std::vector<char> MutateOmniAtmosphereOps(const std::vector<char> &ops, Mutator mutate)
{
	return MutateOmniNodeOps(ops, "omniAtmosphere", mutate);
}

void AppendProbeDoubleLittleEndian(std::vector<unsigned char> &target, double value)
{
	const auto bits = std::bit_cast<std::uint64_t>(value);
	for (unsigned shift = 0; shift < 64; shift += 8)
		target.push_back(static_cast<unsigned char>((bits >> shift) & UINT64_C(0xFF)));
}

bool IsNaNBits(double value)
{
	const auto bits = std::bit_cast<std::uint64_t>(value);
	return (bits & UINT64_C(0x7FF0000000000000)) == UINT64_C(0x7FF0000000000000) &&
		(bits & UINT64_C(0x000FFFFFFFFFFFFF)) != 0;
}

std::vector<char> MakeLegacyAtmosphereOps(
	const std::vector<char> &base,
	const std::vector<double> &species,
	double momentumX,
	double momentumY,
	double totalEnergy,
	double condensedWaterDensity,
	unsigned char valid)
{
	return MutateOmniAtmosphereOps(base, [&](Bson &omniNode) {
		omniNode["stateVersion"] = GameSave::OmniAtmosphereLegacyStateVersion;
		std::vector<unsigned char> data;
		data.reserve((species.size() + 4) * sizeof(double));
		for (double value : species)
			AppendProbeDoubleLittleEndian(data, value);
		AppendProbeDoubleLittleEndian(data, momentumX);
		AppendProbeDoubleLittleEndian(data, momentumY);
		AppendProbeDoubleLittleEndian(data, totalEnergy);
		AppendProbeDoubleLittleEndian(data, condensedWaterDensity);
		omniNode["data"] = std::move(data);
		omniNode["valid"] = std::vector<unsigned char>{ valid };
	});
}

bool RejectsCorruptOps(const std::vector<char> &ops)
{
	if (ops.empty())
		return false;
	try
	{
		GameSave invalid(ops);
	}
	catch (const ParseException &)
	{
		return true;
	}
	return false;
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
	// Keep the synthetic payload above the H2O latent-energy floor used by the
	// strict atmosphere validator (the fixture is still deliberately modest).
	save.omniAtmosphereTotalEnergy.resize(cellCount, 500000.0);
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

bool SameSourceLedger(const OmniAtmosphereLedger &left, const OmniAtmosphereLedger &right)
{
	return left.sourceMassKg == right.sourceMassKg &&
		left.sourceMomentumX == right.sourceMomentumX &&
		left.sourceMomentumY == right.sourceMomentumY &&
		left.sourceEnergyJ == right.sourceEnergyJ &&
		left.sourceSpeciesMassKg == right.sourceSpeciesMassKg;
}

bool PendingSourcesAreZero(const OmniAtmosphereRegionRestoreToken &token)
{
	return token.pendingSourceMassKg == 0.0 && token.pendingSourceMomentumX == 0.0 &&
		token.pendingSourceMomentumY == 0.0 && token.pendingSourceEnergyJ == 0.0 &&
		std::all_of(token.pendingSourceSpeciesMassKg.begin(), token.pendingSourceSpeciesMassKg.end(),
			[](double value) { return value == 0.0; });
}

bool SamePendingSources(
	const OmniAtmosphereRegionRestoreToken &left,
	const OmniAtmosphereRegionRestoreToken &right)
{
	return left.pendingSourceMassKg == right.pendingSourceMassKg &&
		left.pendingSourceMomentumX == right.pendingSourceMomentumX &&
		left.pendingSourceMomentumY == right.pendingSourceMomentumY &&
		left.pendingSourceEnergyJ == right.pendingSourceEnergyJ &&
		left.pendingSourceSpeciesMassKg == right.pendingSourceSpeciesMassKg;
}

bool NearlySame(double left, double right)
{
	const double scale = std::max({ std::abs(left), std::abs(right), 1.0e-300 });
	return std::abs(left - right) <= 64.0 * std::numeric_limits<double>::epsilon() * scale;
}

}

int Run()
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

	// Build a one-cell historical v2 payload explicitly. The binary layout is
	// the immutable v2 cell contract; only the surrounding OPS container comes
	// from the current writer.
	GameSave legacySeed(Vec2<int>{ 1, 1 });
	legacySeed.omniSimulationMode = OMNI_ENHANCED;
	ConfigurePayload(legacySeed);
	const auto legacyBase = legacySeed.Serialise().second;
	const std::vector<double> legacyN2{ 1.0, 0.0, 0.0, 0.0, 0.0 };
	const auto legacyV2 = MakeLegacyAtmosphereOps(
		legacyBase, legacyN2, 0.0, 0.0, 0.01, 0.0, 1);
	if (legacyV2.empty())
		return Fail("could not construct the historical v2 atmosphere fixture");
	GameSave migratedV2A(legacyV2);
	GameSave migratedV2B(legacyV2);
	const double expectedN2Energy =
		OmniDefaultAtmosphereSpecies()[OMNI_SPECIES_N2].specificHeatCpJKgK -
		8.31446261815324 /
			OmniDefaultAtmosphereSpecies()[OMNI_SPECIES_N2].molarMassKgPerMol;
	if (!migratedV2A.hasOmniAtmosphereState ||
		migratedV2A.omniAtmosphereStateVersion != GameSave::OmniAtmosphereStateVersion ||
		!migratedV2A.omniAtmosphereMigratedFromLegacyV2 ||
		migratedV2A.omniAtmosphereSpeciesMassDensity != legacyN2 ||
		!NearlySame(migratedV2A.omniAtmosphereTotalEnergy[0], expectedN2Energy) ||
		migratedV2A.omniAtmosphereSpeciesMassDensity !=
			migratedV2B.omniAtmosphereSpeciesMassDensity ||
		migratedV2A.omniAtmosphereTotalEnergy != migratedV2B.omniAtmosphereTotalEnergy)
	{
		return Fail("historical v2 payload was not deterministically canonicalized to v3");
	}
	auto migratedV2Simulation = Simulation::Factory();
	auto migratedV2SimulationB = Simulation::Factory();
	migratedV2Simulation->SetOmniSimulationMode(OMNI_ENHANCED);
	migratedV2SimulationB->SetOmniSimulationMode(OMNI_ENHANCED);
	migratedV2Simulation->Load(&migratedV2A, true, { 0, 0 });
	migratedV2SimulationB->Load(&migratedV2B, true, { 0, 0 });
	const auto migratedV2Primitive = migratedV2Simulation->omniAtmosphere->Primitive(0, 0);
	const double expectedN2Pressure =
		8.31446261815324 /
		OmniDefaultAtmosphereSpecies()[OMNI_SPECIES_N2].molarMassKgPerMol;
	if (std::string_view(migratedV2Simulation->GetOmniAtmospherePersistenceStatus()) !=
			"migrated_v2_to_v3" ||
		std::abs(migratedV2Primitive.temperature - 1.0) > 1.0e-12 ||
		!NearlySame(migratedV2Primitive.pressure, expectedN2Pressure) ||
		!SameAtmosphere(*migratedV2Simulation->omniAtmosphere,
			*migratedV2SimulationB->omniAtmosphere))
	{
		return Fail("v2 migration did not produce the expected 1 K N2 canonical state");
	}
	AdvanceOneTick(*migratedV2Simulation);
	AdvanceOneTick(*migratedV2SimulationB);
	if (!SameAtmosphere(*migratedV2Simulation->omniAtmosphere,
		*migratedV2SimulationB->omniAtmosphere))
	{
		return Fail("v2 migrated state did not continue deterministically");
	}
	const auto migratedV3Bytes = migratedV2A.Serialise().second;
	if (migratedV3Bytes.empty())
		return Fail("canonicalized v2 state could not be serialized as v3");
	GameSave reparsedMigratedV3(migratedV3Bytes);
	if (reparsedMigratedV3.omniAtmosphereStateVersion != GameSave::OmniAtmosphereStateVersion ||
		reparsedMigratedV3.omniAtmosphereMigratedFromLegacyV2 ||
		reparsedMigratedV3.omniAtmosphereSpeciesMassDensity !=
			migratedV2A.omniAtmosphereSpeciesMassDensity ||
		reparsedMigratedV3.omniAtmosphereTotalEnergy != migratedV2A.omniAtmosphereTotalEnergy)
	{
		return Fail("v2 migration did not reserialize as a byte-stable canonical v3 payload");
	}

	const std::vector<double> lowDensitySpecies{ 2.0e-20, 3.0e-20, 0.0, 0.0, 0.0 };
	GameSave migratedLowDensity(MakeLegacyAtmosphereOps(
		legacyBase, lowDensitySpecies, 0.0, 0.0, 1.0e-12, 0.0, 1));
	double migratedDensity = 0.0;
	for (double value : migratedLowDensity.omniAtmosphereSpeciesMassDensity)
		migratedDensity += value;
	const double migratedN2Fraction =
		migratedLowDensity.omniAtmosphereSpeciesMassDensity[OMNI_SPECIES_N2] / migratedDensity;
	if (migratedDensity < OmniAtmosphereConfig{}.densityFloor ||
		std::abs(migratedN2Fraction - 0.4) > 1.0e-12 ||
		!OmniValidateSerializedAtmosphereCell(
			OmniAtmosphereConfig{}, migratedLowDensity.omniAtmosphereSpeciesMassDensity,
			migratedLowDensity.omniAtmosphereMomentumX[0],
			migratedLowDensity.omniAtmosphereMomentumY[0],
			migratedLowDensity.omniAtmosphereTotalEnergy[0],
			migratedLowDensity.omniAtmosphereCondensedWaterDensity[0], true))
	{
		return Fail("low-density v2 cell did not preserve composition at the v3 floor");
	}

	const std::vector<double> latentKineticSpecies{ 1.0, 0.0, 0.0, 0.0, 0.1 };
	GameSave migratedLatentKinetic(MakeLegacyAtmosphereOps(
		legacyBase, latentKineticSpecies, 1000.0, -500.0, 0.01, 0.0, 1));
	if (migratedLatentKinetic.omniAtmosphereMomentumX[0] != 1000.0 ||
		migratedLatentKinetic.omniAtmosphereMomentumY[0] != -500.0 ||
		migratedLatentKinetic.omniAtmosphereSpeciesMassDensity != latentKineticSpecies ||
		!(migratedLatentKinetic.omniAtmosphereTotalEnergy[0] > 0.01) ||
		!OmniValidateSerializedAtmosphereCell(
			OmniAtmosphereConfig{}, migratedLatentKinetic.omniAtmosphereSpeciesMassDensity,
			1000.0, -500.0, migratedLatentKinetic.omniAtmosphereTotalEnergy[0], 0.0, true))
	{
		return Fail("v2 migration did not repair latent/kinetic energy atomically");
	}

	GameSave migratedInvalidPlaceholder(MakeLegacyAtmosphereOps(
		legacyBase, std::vector<double>(OMNI_COMMON_SPECIES_COUNT, 0.0),
		12.0, -3.0, -5.0, 0.0, 0));
	if (migratedInvalidPlaceholder.omniAtmosphereCellValid !=
			std::vector<unsigned char>{ 0 } ||
		migratedInvalidPlaceholder.omniAtmosphereTotalEnergy != std::vector<double>{ 0.0 } ||
		migratedInvalidPlaceholder.omniAtmosphereMomentumX != std::vector<double>{ 12.0 } ||
		migratedInvalidPlaceholder.omniAtmosphereMomentumY != std::vector<double>{ -3.0 })
	{
		return Fail("invalid-mask v2 placeholder was not canonicalized without losing momentum");
	}
	GameSave migratedNonBinaryMask(MakeLegacyAtmosphereOps(
		legacyBase, legacyN2, 0.0, 0.0, 0.01, 0.0, 2));
	if (migratedNonBinaryMask.omniAtmosphereCellValid != std::vector<unsigned char>{ 1 })
		return Fail("v2 non-binary validity mask did not retain legacy normalization");
	auto missingLegacyMask = MutateOmniAtmosphereOps(legacyV2, [](Bson &omniNode) {
		omniNode.As<Bson::Object>().erase(ByteString("valid"));
	});
	GameSave migratedMissingLegacyMask(missingLegacyMask);
	if (migratedMissingLegacyMask.omniAtmosphereCellValid != std::vector<unsigned char>{ 1 })
		return Fail("v2 payload without a validity mask did not retain legacy all-valid semantics");

	auto strictRetag = MutateOmniAtmosphereOps(legacyV2, [](Bson &omniNode) {
		omniNode["stateVersion"] = GameSave::OmniAtmosphereStateVersion;
	});
	if (!RejectsCorruptOps(strictRetag))
		return Fail("v2-low-energy bytes retagged as v3 were not rejected");
	auto futureVersion = MutateOmniAtmosphereOps(serialised, [](Bson &omniNode) {
		omniNode["stateVersion"] = GameSave::OmniAtmosphereStateVersion + 1;
	});
	if (!RejectsCorruptOps(futureVersion))
		return Fail("future OmniAtmosphere state version was not rejected");
	auto badLegacyLayout = MutateOmniAtmosphereOps(legacyV2, [](Bson &omniNode) {
		omniNode["layout"] = ByteString("wrong_legacy_layout");
	});
	if (!RejectsCorruptOps(badLegacyLayout))
		return Fail("v2 payload with a wrong layout was not rejected");
	auto badLegacySize = MutateOmniAtmosphereOps(legacyV2, [](Bson &omniNode) {
		auto &data = omniNode["data"].As<Bson::User>();
		data.pop_back();
	});
	if (!RejectsCorruptOps(badLegacySize))
		return Fail("v2 payload with a wrong data size was not rejected");
	auto badLegacySpecies = MutateOmniAtmosphereOps(legacyV2, [](Bson &omniNode) {
		omniNode["species"].As<Bson::Array>()[0] = ByteString("UNKNOWN");
	});
	if (!RejectsCorruptOps(badLegacySpecies))
		return Fail("v2 payload with a wrong species registry was not rejected");
	if (!RejectsCorruptOps(MakeLegacyAtmosphereOps(
			legacyBase, std::vector<double>(OMNI_COMMON_SPECIES_COUNT, 0.0),
			0.0, 0.0, 1.0, 0.0, 1)) ||
		!RejectsCorruptOps(MakeLegacyAtmosphereOps(
			legacyBase, legacyN2, 0.0, 0.0, 0.0, 0.0, 1)) ||
		!RejectsCorruptOps(MakeLegacyAtmosphereOps(
			legacyBase, std::vector<double>{ -1.0, 0.0, 0.0, 0.0, 0.0 },
			0.0, 0.0, 1.0, 0.0, 1)) ||
		!RejectsCorruptOps(MakeLegacyAtmosphereOps(
			legacyBase, legacyN2, std::numeric_limits<double>::quiet_NaN(),
			0.0, 1.0, 0.0, 1)) ||
		!RejectsCorruptOps(MakeLegacyAtmosphereOps(
			legacyBase, legacyN2, 0.0, 0.0, 1.0, -1.0, 1)))
	{
		return Fail("invalid historical v2 conservative state was accepted");
	}
	std::vector<double> atomicSpecies = legacyN2;
	const auto atomicSpeciesBefore = atomicSpecies;
	double atomicEnergy = 0.01;
	if (OmniMigrateLegacySerializedAtmosphereCellV2(
			OmniAtmosphereConfig{}, atomicSpecies,
			std::numeric_limits<double>::max(), 0.0, atomicEnergy, 0.0, true) ||
		atomicSpecies != atomicSpeciesBefore || atomicEnergy != 0.01)
	{
		return Fail("failed v2 migration partially modified its caller-owned state");
	}
	GameSave manualLegacyV2 = *save;
	manualLegacyV2.omniAtmosphereStateVersion = GameSave::OmniAtmosphereLegacyStateVersion;
	if (manualLegacyV2.Serialise().first || !manualLegacyV2.Serialise().second.empty())
		return Fail("uncanonicalized in-memory v2 payload was serialized");

	auto invalidLayoutOps = MutateOmniAtmosphereOps(serialised, [](Bson &omniNode) {
		omniNode["layout"] = ByteString("unknown_atmosphere_layout");
	});
	if (!RejectsCorruptOps(invalidLayoutOps))
		return Fail("unknown OmniAtmosphere OPS layout was not rejected");
	auto invalidValidityOps = MutateOmniAtmosphereOps(serialised, [](Bson &omniNode) {
		auto *validNode = omniNode.Get("valid");
		if (!validNode || !validNode->Is<Bson::User>() || validNode->As<Bson::User>().empty())
			throw std::runtime_error("missing validity mask");
		omniNode["valid"].As<Bson::User>()[0] = 2;
	});
	if (!RejectsCorruptOps(invalidValidityOps))
		return Fail("non-binary OmniAtmosphere validity mask was not rejected");
	auto missingCurrentValidity = MutateOmniAtmosphereOps(serialised, [](Bson &omniNode) {
		omniNode.As<Bson::Object>().erase(ByteString("valid"));
	});
	if (!RejectsCorruptOps(missingCurrentValidity))
		return Fail("v3 payload without a validity mask was not rejected");

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
	const auto &loadedLedger = loadedA->omniAtmosphere->Ledger();
	const auto loadedPending = loadedA->omniAtmosphere->BeginRegionStateRestore();
	if (std::string_view(loadedA->GetOmniAtmospherePersistenceStatus()) != "loaded_v3" ||
		!SameCell(*source->omniAtmosphere, *loadedA->omniAtmosphere, 5, 7) ||
		!SameAtmosphere(*loadedA->omniAtmosphere, *loadedB->omniAtmosphere) ||
		!PendingSourcesAreZero(loadedPending) ||
		loadedLedger.sourceMassKg != 0.0 || loadedLedger.sourceMomentumX != 0.0 ||
		loadedLedger.sourceMomentumY != 0.0 || loadedLedger.sourceEnergyJ != 0.0 ||
		std::any_of(loadedLedger.sourceSpeciesMassKg.begin(),
			loadedLedger.sourceSpeciesMassKg.end(), [](double value) { return value != 0.0; }))
	{
		return Fail("Simulation load did not restore an exact ledger-neutral atmosphere baseline");
	}
	AdvanceOneTick(*loadedA);
	AdvanceOneTick(*loadedB);
	if (!SameAtmosphere(*loadedA->omniAtmosphere, *loadedB->omniAtmosphere) ||
		!SameSourceLedger(loadedA->omniAtmosphere->Ledger(), loadedB->omniAtmosphere->Ledger()))
		return Fail("save/load continuation was not deterministic");

	GameSave regionPayload(Vec2<int>{ 1, 1 });
	regionPayload.omniSimulationMode = OMNI_ENHANCED;
	ConfigurePayload(regionPayload);
	regionPayload.omniAtmosphereSpeciesMassDensity[OMNI_SPECIES_H2O] = 0.0;
	regionPayload.omniAtmosphereCondensedWaterDensity[0] = 0.0;
	auto regionTarget = Simulation::Factory();
	regionTarget->SetOmniSimulationMode(OMNI_ENHANCED);
	regionTarget->omniAtmosphere->AddEnergyDensity(2, 2, 1234.5);
	regionTarget->omniAtmosphere->AddSpeciesMassDensity(2, 2, OMNI_SPECIES_CO2, 0.00125);
	const auto regionPendingBefore = regionTarget->omniAtmosphere->BeginRegionStateRestore();
	regionTarget->Load(&regionPayload, true, { 20, 12 });
	const auto regionPendingAfter = regionTarget->omniAtmosphere->BeginRegionStateRestore();
	if (!SamePendingSources(regionPendingBefore, regionPendingAfter))
		return Fail("region atmosphere restore erased an unrelated pending source");

	GameSave invalidRegionPayload = regionPayload;
	invalidRegionPayload.omniAtmosphereCellValid.assign(1, 0);
	auto invalidRegionTarget = Simulation::Factory();
	invalidRegionTarget->SetOmniSimulationMode(OMNI_ENHANCED);
	invalidRegionTarget->omniAtmosphere->AddEnergyDensity(3, 3, 4321.0);
	const auto invalidRegionPendingBefore =
		invalidRegionTarget->omniAtmosphere->BeginRegionStateRestore();
	invalidRegionTarget->Load(&invalidRegionPayload, true, { 22, 14 });
	if (!SamePendingSources(
			invalidRegionPendingBefore,
			invalidRegionTarget->omniAtmosphere->BeginRegionStateRestore()))
	{
		return Fail("all-invalid atmosphere region erased a pending source");
	}

	auto emptyRegionTarget = Simulation::Factory();
	emptyRegionTarget->SetOmniSimulationMode(OMNI_ENHANCED);
	emptyRegionTarget->omniAtmosphere->AddSpeciesMassDensity(4, 4, OMNI_SPECIES_CO2, 0.0005);
	const auto emptyRegionPendingBefore = emptyRegionTarget->omniAtmosphere->BeginRegionStateRestore();
	emptyRegionTarget->Load(&regionPayload, true, { XCELLS + 5, YCELLS + 5 });
	if (!SamePendingSources(
			emptyRegionPendingBefore,
			emptyRegionTarget->omniAtmosphere->BeginRegionStateRestore()))
	{
		return Fail("empty atmosphere target region erased a pending source");
	}

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
	const int undoCarbon = undoSource->create_part(-1, 54, 50, PT_COAL);
	if (undoCarbon < 0)
		return Fail("could not create the undo carbon fixture");
	undoSource->parts[undoWater].temp = 360.0f;
	const double undoWaterMass = undoSource->GetOmniWaterParcelMassKg(undoWater);
	const double undoWaterEnthalpy =
		undoSource->GetOmniWaterParcelSpecificEnthalpyJPerKg(undoWater);
	const double undoCarbonMass = undoSource->GetOmniCarbonParcelMassKg(undoCarbon);
	const auto oldSnapshot = undoSource->CreateSnapshot();
	undoSource->omniAtmosphere->AddSpeciesMassDensity(5, 7, OMNI_SPECIES_H2O, 0.125);
	undoSource->omniAtmosphere->AddEnergyDensity(5, 7, 10000.0);
	undoSource->parts[undoWater].temp = 450.0f;
	undoSource->kill_part(undoWater);
	undoSource->kill_part(undoCarbon);
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
		undoSource->GetOmniWaterParcelMassKg(undoWater) != undoWaterMass ||
		undoSource->GetOmniWaterParcelSpecificEnthalpyJPerKg(undoWater) != undoWaterEnthalpy ||
		undoSource->parts[undoCarbon].type != PT_COAL ||
		undoSource->GetOmniCarbonParcelMassKg(undoCarbon) != undoCarbonMass)
	{
		return Fail("undo restore did not recover Enhanced water/carbon ownership state");
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

	const auto smallRegion = RectSized(
		Vec2<int>{ 0, 0 }, Vec2<int>{ CELL * 2, CELL * 2 });
	auto corruptSaveBoundary = Simulation::Factory();
	corruptSaveBoundary->SetOmniSimulationMode(OMNI_ENHANCED);
	auto &corruptLiveState = const_cast<OmniAtmosphereConservative &>(
		corruptSaveBoundary->omniAtmosphere->State(0, 0));
	const double corruptEnergyBefore = corruptLiveState.totalEnergy;
	corruptLiveState.density = std::numeric_limits<double>::quiet_NaN();
	auto pressureFreeCorruptSave = corruptSaveBoundary->Save(false, smallRegion);
	if (!pressureFreeCorruptSave || pressureFreeCorruptSave->hasOmniAtmosphereState ||
		!IsNaNBits(corruptSaveBoundary->omniAtmosphere->State(0, 0).density) ||
		corruptSaveBoundary->omniAtmosphere->State(0, 0).totalEnergy != corruptEnergyBefore)
	{
		std::cerr << "pressure-free corrupt values: save=" << bool(pressureFreeCorruptSave)
			<< " payload=" << (pressureFreeCorruptSave ? pressureFreeCorruptSave->hasOmniAtmosphereState : false)
			<< " density=" << corruptSaveBoundary->omniAtmosphere->State(0, 0).density
			<< " energy_before=" << corruptEnergyBefore
			<< " energy_after=" << corruptSaveBoundary->omniAtmosphere->State(0, 0).totalEnergy
			<< '\n';
		return Fail("pressure-free save inspected or modified corrupt atmosphere state");
	}
	if (corruptSaveBoundary->Save(true, smallRegion) ||
		!IsNaNBits(corruptSaveBoundary->omniAtmosphere->State(0, 0).density) ||
		corruptSaveBoundary->omniAtmosphere->State(0, 0).totalEnergy != corruptEnergyBefore)
	{
		return Fail("pressure save did not reject corrupt live atmosphere transactionally");
	}

	auto regionalSaveBoundary = Simulation::Factory();
	regionalSaveBoundary->SetOmniSimulationMode(OMNI_ENHANCED);
	constexpr std::size_t outsideX = 8;
	constexpr std::size_t outsideY = 8;
	regionalSaveBoundary->omniAtmosphere->SetCondensedWaterDensity(
		outsideX, outsideY, 1000.0);
	std::vector<double> outsideSpecies(
		regionalSaveBoundary->omniAtmosphere->SpeciesCount(), 0.0);
	for (std::size_t species = 0; species < outsideSpecies.size(); ++species)
	{
		outsideSpecies[species] = regionalSaveBoundary->omniAtmosphere->SpeciesMassDensity(
			outsideX, outsideY, species);
	}
	const auto outsideState = regionalSaveBoundary->omniAtmosphere->State(outsideX, outsideY);
	double outsideCanonicalEnergy = 1.0;
	if (!OmniMigrateLegacySerializedAtmosphereCellV2(
			regionalSaveBoundary->omniAtmosphere->Config(), outsideSpecies,
			outsideState.momentumX, outsideState.momentumY, outsideCanonicalEnergy,
			regionalSaveBoundary->omniAtmosphere->Primitive(
				outsideX, outsideY).condensedWaterDensity, true))
	{
		return Fail("could not construct regional save side-effect fixture");
	}
	auto &outsideMutableState = const_cast<OmniAtmosphereConservative &>(
		regionalSaveBoundary->omniAtmosphere->State(outsideX, outsideY));
	outsideMutableState.totalEnergy = std::nextafter(
		outsideCanonicalEnergy, -std::numeric_limits<double>::infinity());
	const double outsideEnergyBeforeRegionalSave = outsideMutableState.totalEnergy;
	auto regionalPressureSave = regionalSaveBoundary->Save(true, smallRegion);
	if (!regionalPressureSave || !regionalPressureSave->hasOmniAtmosphereState ||
		regionalSaveBoundary->omniAtmosphere->State(
			outsideX, outsideY).totalEnergy != outsideEnergyBeforeRegionalSave)
	{
		return Fail("regional pressure save modified an unrelated atmosphere cell");
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
	transformed.particlesCount = 1;
	transformed.particles[0].type = PT_WATR;
	transformed.particles[0].x = 1.0f;
	transformed.particles[0].y = 1.0f;
	transformed.hasOmniWaterParcelState = true;
	transformed.omniWaterParcelStateVersion = GameSave::OmniWaterParcelStateVersion;
	transformed.omniWaterParcelMassKg = { 2.5e-6 };
	transformed.omniWaterParcelSpecificEnthalpyJPerKg = { 765432.125 };
	transformed.Transform(Mat2<int>::CCW, { 0, 0 });
	const std::size_t rotatedCell = 3;
	if (transformed.blockSize != Vec2<int>{ 3, 2 } ||
		transformed.omniAtmosphereSpeciesMassDensity[rotatedCell * OMNI_COMMON_SPECIES_COUNT] != oldSpecies[0] ||
		transformed.omniAtmosphereMomentumX[rotatedCell] != oldMomentumY[0] ||
		transformed.omniAtmosphereMomentumY[rotatedCell] != -oldMomentumX[0] ||
		transformed.omniWaterParcelMassKg != std::vector<double>{ 2.5e-6 } ||
		transformed.omniWaterParcelSpecificEnthalpyJPerKg != std::vector<double>{ 765432.125 })
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
	if (corrupt.Serialise().first || !corrupt.Serialise().second.empty())
		return Fail("invalid atmosphere state was not rejected before OPS serialization");
	GameSave nonfinite = *save;
	nonfinite.omniAtmosphereMomentumX[0] = std::numeric_limits<double>::quiet_NaN();
	if (nonfinite.Serialise().first || !nonfinite.Serialise().second.empty())
		return Fail("non-finite atmosphere state was not rejected before OPS serialization");
	GameSave latentDeficient = *save;
	const std::size_t speciesCount = latentDeficient.omniAtmosphereSpecies.size();
	std::fill_n(latentDeficient.omniAtmosphereSpeciesMassDensity.begin(), speciesCount, 0.0);
	latentDeficient.omniAtmosphereSpeciesMassDensity[OMNI_SPECIES_N2] = 1.0;
	latentDeficient.omniAtmosphereSpeciesMassDensity[OMNI_SPECIES_H2O] = 0.1;
	latentDeficient.omniAtmosphereMomentumX[0] = 0.0;
	latentDeficient.omniAtmosphereMomentumY[0] = 0.0;
	latentDeficient.omniAtmosphereCondensedWaterDensity[0] = 0.0;
	latentDeficient.omniAtmosphereTotalEnergy[0] =
		0.5 * latentDeficient.omniAtmosphereSpeciesMassDensity[OMNI_SPECIES_H2O] *
		OmniThermal::LatentHeatVaporizationJPerKg;
	if (latentDeficient.Serialise().first || !latentDeficient.Serialise().second.empty())
		return Fail("positive latent-energy-deficient atmosphere state was serialized");
	GameSave belowPressureFloor = *save;
	std::fill_n(belowPressureFloor.omniAtmosphereSpeciesMassDensity.begin(), speciesCount, 0.0);
	belowPressureFloor.omniAtmosphereSpeciesMassDensity[OMNI_SPECIES_N2] = 1.0e-9;
	belowPressureFloor.omniAtmosphereMomentumX[0] = 0.0;
	belowPressureFloor.omniAtmosphereMomentumY[0] = 0.0;
	belowPressureFloor.omniAtmosphereCondensedWaterDensity[0] = 0.0;
	belowPressureFloor.omniAtmosphereTotalEnergy[0] = 1.5e-3;
	if (belowPressureFloor.Serialise().first || !belowPressureFloor.Serialise().second.empty())
		return Fail("positive atmosphere state below the pressure floor was serialized");
	auto invalidWaterSave = undoSource->Save(true, RES.OriginRect());
	if (!invalidWaterSave || invalidWaterSave->omniWaterParcelMassKg.empty())
		return Fail("could not construct a water OPS write-rejection fixture");
	GameSave invalidWater = *invalidWaterSave;
	invalidWater.omniWaterParcelSpecificEnthalpyJPerKg.assign(
		invalidWater.omniWaterParcelMassKg.size(), std::numeric_limits<double>::quiet_NaN());
	if (invalidWater.Serialise().first || !invalidWater.Serialise().second.empty())
		return Fail("non-finite water enthalpy was not rejected before OPS serialization");
	if (!invalidWaterSave->hasOmniCarbonParcelState || invalidWaterSave->omniCarbonParcelMassKg.empty())
		return Fail("could not construct a carbon OPS write-rejection fixture");
	GameSave invalidCarbon = *invalidWaterSave;
	invalidCarbon.omniCarbonParcelMassKg[undoCarbon] =
		std::numeric_limits<double>::quiet_NaN();
	if (invalidCarbon.Serialise().first || !invalidCarbon.Serialise().second.empty())
		return Fail("non-finite carbon mass was not rejected before OPS serialization");

	const auto carbonSerialised = invalidWaterSave->Serialise();
	if (carbonSerialised.second.empty())
		return Fail("serialising the carbon parcel payload failed");
	for (const auto *nodeName : {
		"omniWaterParcels", "omniCarbonParcels", "omniSolutionParcels", "omniCorrosion" })
	{
		auto invalidSidecarLayout = MutateOmniNodeOps(
			carbonSerialised.second, nodeName, [](Bson &node) {
				node["layout"] = ByteString("unknown_sidecar_layout");
			});
		if (!RejectsCorruptOps(invalidSidecarLayout))
			return Fail("unknown Omni sidecar OPS layout was not rejected");
	}
	GameSave carbonParsed(carbonSerialised.second);
	if (!carbonParsed.hasOmniCarbonParcelState ||
		carbonParsed.omniCarbonParcelStateVersion != GameSave::OmniCarbonParcelStateVersion ||
		carbonParsed.omniCarbonParcelMassKg != invalidWaterSave->omniCarbonParcelMassKg)
	{
		return Fail("OPS parse did not preserve the exact carbon parcel payload");
	}
	auto carbonLoaded = Simulation::Factory();
	carbonLoaded->SetOmniSimulationMode(OMNI_ENHANCED);
	carbonLoaded->Load(&carbonParsed, true, { 0, 0 });
	int loadedCarbon = -1;
	for (int i = 0; i < carbonLoaded->parts.active; ++i)
	{
		if (carbonLoaded->parts[i].type == PT_COAL)
		{
			loadedCarbon = i;
			break;
		}
	}
	if (loadedCarbon < 0 || carbonLoaded->GetOmniCarbonParcelMassKg(loadedCarbon) != undoCarbonMass)
		return Fail("Simulation load did not restore carbon parcel ownership");

	std::cout << "omni_save_roundtrip_probe_pass=true\n";
	std::cout << "ops_state_version=" << GameSave::OmniAtmosphereStateVersion << '\n';
	std::cout << "species_channels=" << parsed.omniAtmosphereSpecies.size() << '\n';
	std::cout << "exact_f64_roundtrip=true\n";
	std::cout << "legacy_v2_migrated_to_v3=true\n";
	std::cout << "legacy_v2_migration_deterministic=true\n";
	std::cout << "legacy_v2_migration_atomic=true\n";
	std::cout << "legacy_v2_invalid_placeholder_canonicalized=true\n";
	std::cout << "legacy_v2_nonbinary_mask_normalized=true\n";
	std::cout << "strict_v3_retag_rejected=true\n";
	std::cout << "future_atmosphere_version_rejected=true\n";
	std::cout << "deterministic_continuation=true\n";
	std::cout << "region_restore_preserves_pending_sources=true\n";
	std::cout << "all_invalid_region_preserves_pending_sources=true\n";
	std::cout << "empty_region_preserves_pending_sources=true\n";
	std::cout << "fresh_full_load_source_neutral=true\n";
	std::cout << "snapshot_undo_roundtrip=true\n";
	std::cout << "snapshot_delta_roundtrip=true\n";
	std::cout << "water_snapshot_enthalpy_roundtrip=true\n";
	std::cout << "classic_snapshot_hash_compatible=true\n";
	std::cout << "classic_payload_omitted=true\n";
	std::cout << "region_state_omitted=true\n";
	std::cout << "pressure_save_corrupt_state_rejected=true\n";
	std::cout << "pressure_free_save_state_untouched=true\n";
	std::cout << "regional_pressure_save_nonlocal_side_effect=false\n";
	std::cout << "legacy_projection_migration=true\n";
	std::cout << "transform_momentum=true\n";
	std::cout << "transform_water_enthalpy_alignment=true\n";
	std::cout << "unknown_species_rejected=true\n";
	std::cout << "malformed_payload_rejected=true\n";
	std::cout << "invalid_ops_write_rejected=true\n";
	std::cout << "invalid_ops_layout_rejected=true\n";
	std::cout << "invalid_sidecar_ops_layouts_rejected=true\n";
	std::cout << "invalid_ops_validity_mask_rejected=true\n";
	std::cout << "carbon_sidecar_roundtrip=true\n";
	return 0;
}

int main()
{
	try
	{
		return Run();
	}
	catch (const std::exception &error)
	{
		std::cerr << "omni-save-roundtrip-probe: EXCEPTION " << error.what() << std::endl;
		return 1;
	}
}
