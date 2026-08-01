#include "bzip2/bz2wrap.h"
#include "client/GameSave.h"
#include "common/Bson.h"
#include "simulation/ElementClasses.h"
#include "simulation/OmniMetallurgy.h"
#include "simulation/SimulationData.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace
{
int Fail(std::string_view message)
{
	std::cerr << "high-id-save-probe: FAIL " << message << std::endl;
	return 1;
}

std::vector<char> DecompressPayload(std::vector<char> const &ops)
{
	if (ops.size() <= 12 || std::string_view(ops.data(), 4) != "OPS1")
	{
		return {};
	}
	auto payloadSize = static_cast<std::uint32_t>(static_cast<unsigned char>(ops[8])) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[9])) << 8U) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[10])) << 16U) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(ops[11])) << 24U);
	std::vector<char> payload;
	if (BZ2WDecompress(
			payload,
			std::span(ops.data() + 12, ops.size() - 12),
			payloadSize) != BZ2WDecompressOk)
	{
		return {};
	}
	return payload;
}

bool Contains(std::vector<char> const &data, std::string_view marker)
{
	return std::search(data.begin(), data.end(), marker.begin(), marker.end()) != data.end();
}

Bson ProbeOpsNonconformance()
{
	Bson nonconformance(Bson::Type::objectValue);
	auto &palette = (nonconformance["palette"] = Bson::Type::arrayValue);
	palette.Append("objectEncodedAsArray");
	return nonconformance;
}

std::vector<char> WithPmapBits(std::vector<char> const &ops, std::int32_t pmapbits)
{
	auto payload = DecompressPayload(ops);
	constexpr std::string_view marker("pmapbits\0", 9);
	auto found = std::search(payload.begin(), payload.end(), marker.begin(), marker.end());
	if (found == payload.end())
	{
		return {};
	}
	auto offset = static_cast<std::size_t>(found - payload.begin()) + marker.size();
	if (offset + sizeof(pmapbits) > payload.size())
	{
		return {};
	}
	for (std::size_t byte = 0; byte < sizeof(pmapbits); ++byte)
	{
		payload[offset + byte] = static_cast<char>(
			(static_cast<std::uint32_t>(pmapbits) >> (byte * 8U)) & 0xFFU);
	}

	std::vector<char> compressed;
	if (BZ2WCompress(compressed, payload) != BZ2WCompressOk)
	{
		return {};
	}
	std::vector<char> mutated(ops.begin(), ops.begin() + 12);
	mutated.insert(mutated.end(), compressed.begin(), compressed.end());
	return mutated;
}

std::vector<char> WithMissingSolderIdentifier(std::vector<char> const &ops)
{
	auto payload = DecompressPayload(ops);
	constexpr std::string_view original("OMNI_PT_SOLD\0", 13);
	constexpr std::string_view replacement("MISSING_SOLD\0", 13);
	static_assert(original.size() == replacement.size());
	auto found = std::search(payload.begin(), payload.end(), original.begin(), original.end());
	if (found == payload.end())
	{
		return {};
	}
	std::copy(replacement.begin(), replacement.end(), found);
	std::vector<char> compressed;
	if (BZ2WCompress(compressed, payload) != BZ2WCompressOk)
	{
		return {};
	}
	std::vector<char> mutated(ops.begin(), ops.begin() + 12);
	mutated.insert(mutated.end(), compressed.begin(), compressed.end());
	return mutated;
}

std::vector<char> WithKnownIdentifierInWiderSourceSlot(std::vector<char> const &ops)
{
	auto payload = DecompressPayload(ops);
	if (payload.empty())
	{
		return {};
	}
	try
	{
		auto nonconformance = ProbeOpsNonconformance();
		auto bson = Bson::Parse(payload, &nonconformance);
		auto const *paletteNode = bson.Get("palette");
		auto const *partsNode = bson.Get("parts");
		if (!paletteNode || !paletteNode->Is<Bson::Object>() ||
			!partsNode || !partsNode->Is<Bson::User>())
		{
			return {};
		}
		auto &palette = bson["palette"].As<Bson::Object>();
		auto solder = palette.find("OMNI_PT_SOLD");
		if (solder == palette.end())
		{
			return {};
		}
		constexpr std::int32_t widerSourceId = 1536;
		bson["pmapbits"] = std::int32_t{ 11 };
		solder->second = widerSourceId;

		// The first fixture is SOLD.  OPS stores its low type byte, two field
		// descriptor bytes, then the optional high type byte.  Both 512 and 1536
		// use that two-byte representation, so only the high byte changes.
		auto &parts = bson["parts"].As<Bson::User>();
		if (parts.size() < 4 || !(parts[2] & 0x40U))
		{
			return {};
		}
		parts[0] = static_cast<unsigned char>(widerSourceId & 0xFF);
		parts[3] = static_cast<unsigned char>((widerSourceId >> 8) & 0xFF);
		payload = bson.Dump(&nonconformance);
	}
	catch (std::exception const &)
	{
		return {};
	}

	std::vector<char> compressed;
	if (BZ2WCompress(compressed, payload) != BZ2WCompressOk)
	{
		return {};
	}
	std::vector<char> mutated(ops.begin(), ops.begin() + 12);
	auto payloadSize = static_cast<std::uint32_t>(payload.size());
	for (std::size_t byte = 0; byte < sizeof(payloadSize); ++byte)
	{
		mutated[8 + byte] = static_cast<char>((payloadSize >> (byte * 8U)) & 0xFFU);
	}
	mutated.insert(mutated.end(), compressed.begin(), compressed.end());
	return mutated;
}

bool RejectsInvalidPmapBits(std::vector<char> const &ops, int value)
{
	auto mutated = WithPmapBits(ops, value);
	if (mutated.empty())
	{
		return false;
	}
	try
	{
		GameSave invalid(mutated);
	}
	catch (ParseException const &error)
	{
		return error.result == ParseException::Corrupt &&
			std::string_view(error.what()).find("Invalid OPS pmapbits value") != std::string_view::npos;
	}
	return false;
}

struct Fixture
{
	int type;
	int ctype;
	int tmp;
	int tmp2;
	int tmp4;
	int life;
};
}

int main()
{
	static_assert(PT_SOLD == 512);
	static_assert(PT_NITI == 520);
	static_assert(PT_RFBK == 532);
	static_assert(PT_CF52 == 588);
	static_assert(PMAPBITS == 10);
	static_assert(PT_NUM == 1024);

	auto simulationData = std::make_unique<SimulationData>();
	if (!simulationData->IsElement(PT_SOLD) ||
		simulationData->elements[PT_SOLD].Identifier != "OMNI_PT_SOLD" ||
		!simulationData->IsElement(PT_RFBK) ||
		simulationData->elements[PT_RFBK].Identifier != "OMNI_PT_RFBK" ||
		!simulationData->IsElement(PT_CF52) ||
		simulationData->elements[PT_CF52].Identifier != "OMNI_PT_CF52")
	{
		return Fail("SOLD=512 RFBK=532 or CF52=588 is not an active stable element");
	}

	constexpr std::array fixtures{
		Fixture{ PT_SOLD, 0, 0, 0, 0, 0 },
		Fixture{ PT_RFBK, 0, 0, 0, 0, 0 },
		Fixture{ PT_CF52, 0, 0, 0, 0, 0 },
		Fixture{ PT_LAVA, PT_CF52, 0, 0, 0, 0 },
		Fixture{ PT_SPRK, PT_CF52, 0, 0, 0, 4 },
		Fixture{ PT_BRMT, PT_NITI, 0, 0, OmniRecoverableScrapMarker, 0 },
		Fixture{ PT_CONV, PT_CF52, PT_CF52, 0, 0, 0 },
		Fixture{ PT_VIRS, 0, 0, PT_CF52, 0, 0 },
	};

	GameSave source(Vec2<int>{ 8, 2 });
	source.pmapbits = PMAPBITS;
	source.particlesCount = static_cast<int>(fixtures.size());
	for (std::size_t index = 0; index < fixtures.size(); ++index)
	{
		auto &particle = source.particles[index];
		auto const &fixture = fixtures[index];
		particle.type = fixture.type;
		particle.ctype = fixture.ctype;
		particle.tmp = fixture.tmp;
		particle.tmp2 = fixture.tmp2;
		particle.tmp4 = fixture.tmp4;
		particle.life = fixture.life;
		particle.temp = 300.0f;
		particle.x = static_cast<float>(2 + index * 3);
		particle.y = 2.0f;
	}

	auto serialised = source.Serialise().second;
	if (serialised.empty())
	{
		return Fail("serialising the high-ID fixture returned no OPS data");
	}
	auto payload = DecompressPayload(serialised);
	if (payload.empty() || !Contains(payload, std::string_view("OMNI_PT_SOLD\0", 13)))
	{
		return Fail("OPS palette does not contain the SOLD stable identifier");
	}

	GameSave loaded(serialised);
	if (loaded.pmapbits != PMAPBITS || loaded.particlesCount != static_cast<int>(fixtures.size()))
	{
		return Fail("OPS metadata or particle count changed after high-ID load");
	}

	GameSave widerSourceFixture(Vec2<int>{ 2, 2 });
	widerSourceFixture.pmapbits = PMAPBITS;
	widerSourceFixture.particlesCount = 1;
	widerSourceFixture.particles[0].type = PT_SOLD;
	widerSourceFixture.particles[0].temp = 300.0f;
	widerSourceFixture.particles[0].x = 2.0f;
	widerSourceFixture.particles[0].y = 2.0f;
	auto widerSourceSlotOps = WithKnownIdentifierInWiderSourceSlot(
		widerSourceFixture.Serialise().second);
	if (widerSourceSlotOps.empty())
	{
		return Fail("could not construct the wider-source-slot fixture");
	}
	GameSave widerSourceSlot(widerSourceSlotOps);
	if (widerSourceSlot.pmapbits != 11 ||
		widerSourceSlot.particles[0].type != PT_SOLD ||
		!widerSourceSlot.missingElements.ids.empty() ||
		!widerSourceSlot.missingElements.identifiers.empty())
	{
		std::cerr << "high-id-save-probe: wider-source diagnostic pmapbits="
			<< widerSourceSlot.pmapbits
			<< " particles=" << widerSourceSlot.particlesCount
			<< " type=" << widerSourceSlot.particles[0].type
			<< " palette=" << widerSourceSlot.palette.size()
			<< " missing_ids=" << widerSourceSlot.missingElements.ids.size()
			<< " missing_identifiers=" << widerSourceSlot.missingElements.identifiers.size()
			<< std::endl;
		if (!widerSourceSlot.palette.empty())
		{
			std::cerr << "high-id-save-probe: wider-source palette identifier="
				<< widerSourceSlot.palette[0].first
				<< " id=" << widerSourceSlot.palette[0].second << std::endl;
		}
		return Fail("known identifier in a wider source slot did not map before range filtering");
	}
	for (std::size_t index = 0; index < fixtures.size(); ++index)
	{
		auto const &particle = loaded.particles[index];
		auto const &fixture = fixtures[index];
		if (particle.type != fixture.type || particle.ctype != fixture.ctype ||
			particle.tmp != fixture.tmp || particle.tmp2 != fixture.tmp2 ||
			particle.tmp4 != fixture.tmp4 || particle.life != fixture.life)
		{
			return Fail("a direct or carried high-ID field changed after OPS load");
		}
	}

	if (!RejectsInvalidPmapBits(serialised, 0) || !RejectsInvalidPmapBits(serialised, 17))
	{
		return Fail("corrupt OPS pmap width was not rejected fail-closed");
	}

	auto missingIdentifierOps = WithMissingSolderIdentifier(serialised);
	if (missingIdentifierOps.empty())
	{
		return Fail("could not construct the missing high-ID identifier fixture");
	}
	GameSave missingIdentifier(missingIdentifierOps);
	if (missingIdentifier.particles[0].type != PT_NONE ||
		missingIdentifier.missingElements.ids.count(PT_SOLD) != 1 ||
		missingIdentifier.missingElements.identifiers.count("MISSING_SOLD") != 1)
	{
		return Fail("missing high-ID identifier was not reported and neutralised");
	}

	std::cout << "high-id-save-probe: PASS pmapbits=" << PMAPBITS
		<< " pt_num=" << PT_NUM
		<< " high_ids=" << PT_SOLD << "-" << PT_CF52
		<< " particles=" << fixtures.size()
		<< " direct_types=3 ctype_carriers=4 tmp_carriers=1 tmp2_carriers=1"
		<< " invalid_pmapbits_rejected=2 missing_identifier_detected=1"
		<< " wider_source_slot_remapped=1" << std::endl;
	return 0;
}
