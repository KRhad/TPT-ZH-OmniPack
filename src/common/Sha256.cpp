#include "Sha256.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace
{
	constexpr std::array<uint32_t, 64> RoundConstants = {
		0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U,
		0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
		0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U,
		0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
		0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
		0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
		0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
		0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
		0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U,
		0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
		0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U,
		0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
		0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U,
		0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
		0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
		0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U,
	};

	constexpr uint32_t RotateRight(uint32_t value, int bits)
	{
		return (value >> bits) | (value << (32 - bits));
	}

	class Sha256State
	{
		std::array<uint32_t, 8> state = {
			0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
			0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U,
		};
		std::array<uint8_t, 64> block{};
		size_t blockSize = 0;
		uint64_t totalBytes = 0;

		void Transform()
		{
			std::array<uint32_t, 64> words{};
			for (size_t i = 0; i < 16; ++i)
			{
				words[i] = uint32_t(block[i * 4]) << 24 |
					uint32_t(block[i * 4 + 1]) << 16 |
					uint32_t(block[i * 4 + 2]) << 8 |
					uint32_t(block[i * 4 + 3]);
			}
			for (size_t i = 16; i < words.size(); ++i)
			{
				auto s0 = RotateRight(words[i - 15], 7) ^ RotateRight(words[i - 15], 18) ^ (words[i - 15] >> 3);
				auto s1 = RotateRight(words[i - 2], 17) ^ RotateRight(words[i - 2], 19) ^ (words[i - 2] >> 10);
				words[i] = words[i - 16] + s0 + words[i - 7] + s1;
			}

			auto a = state[0];
			auto b = state[1];
			auto c = state[2];
			auto d = state[3];
			auto e = state[4];
			auto f = state[5];
			auto g = state[6];
			auto h = state[7];
			for (size_t i = 0; i < words.size(); ++i)
			{
				auto sigma1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
				auto choose = (e & f) ^ (~e & g);
				auto temp1 = h + sigma1 + choose + RoundConstants[i] + words[i];
				auto sigma0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
				auto majority = (a & b) ^ (a & c) ^ (b & c);
				auto temp2 = sigma0 + majority;
				h = g;
				g = f;
				f = e;
				e = d + temp1;
				d = c;
				c = b;
				b = a;
				a = temp1 + temp2;
			}
			state[0] += a;
			state[1] += b;
			state[2] += c;
			state[3] += d;
			state[4] += e;
			state[5] += f;
			state[6] += g;
			state[7] += h;
		}

	public:
		void Update(std::span<const char> data)
		{
			totalBytes += data.size();
			for (char value : data)
			{
				block[blockSize++] = static_cast<uint8_t>(value);
				if (blockSize == block.size())
				{
					Transform();
					blockSize = 0;
				}
			}
		}

		std::array<uint8_t, 32> Finish()
		{
			auto bitLength = totalBytes * 8;
			block[blockSize++] = 0x80;
			if (blockSize > 56)
			{
				while (blockSize < block.size())
					block[blockSize++] = 0;
				Transform();
				blockSize = 0;
			}
			while (blockSize < 56)
				block[blockSize++] = 0;
			for (size_t i = 0; i < 8; ++i)
				block[63 - i] = uint8_t(bitLength >> (i * 8));
			Transform();

			std::array<uint8_t, 32> digest{};
			for (size_t i = 0; i < state.size(); ++i)
			{
				digest[i * 4] = uint8_t(state[i] >> 24);
				digest[i * 4 + 1] = uint8_t(state[i] >> 16);
				digest[i * 4 + 2] = uint8_t(state[i] >> 8);
				digest[i * 4 + 3] = uint8_t(state[i]);
			}
			return digest;
		}
	};
}

ByteString Sha256Hex(std::span<const char> data)
{
	Sha256State hasher;
	hasher.Update(data);
	auto digest = hasher.Finish();
	constexpr char Hex[] = "0123456789ABCDEF";
	ByteString result;
	result.reserve(digest.size() * 2);
	for (auto value : digest)
	{
		result.push_back(Hex[value >> 4]);
		result.push_back(Hex[value & 0x0F]);
	}
	return result;
}
