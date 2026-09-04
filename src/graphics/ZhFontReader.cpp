#include "graphics/ZhFontReader.h"

#include "bzip2/bz2wrap.h"
#include "graphics/font_bz2.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace
{
struct FontRange
{
	uint32_t first;
	uint32_t last;
	size_t offset;
};

std::vector<char> zhFontData;
std::vector<size_t> zhFontPointers;
std::vector<FontRange> zhFontRanges;
bool zhFontInitialised = false;

bool InitZhFontData()
{
	if (zhFontInitialised)
		return true;
	if (BZ2WDecompress(zhFontData,
			reinterpret_cast<const char *>(font_bz2.data.data()),
			font_bz2.data.size(), 64U * 1024U * 1024U) != BZ2WDecompressOk)
		return false;

	const unsigned char *begin = reinterpret_cast<const unsigned char *>(zhFontData.data());
	const unsigned char *ptr = begin;
	const unsigned char *end = begin + zhFontData.size();
	uint32_t rangeFirst = 0;
	uint32_t previous = 0;
	size_t rangeOffset = 0;
	bool haveRange = false;

	while (ptr < end)
	{
		if (end - ptr < 4)
			return false;
		uint32_t codepoint = static_cast<uint32_t>(ptr[0]) |
			(static_cast<uint32_t>(ptr[1]) << 8) |
			(static_cast<uint32_t>(ptr[2]) << 16);
		unsigned int width = ptr[3];
		size_t recordSize = 4U + static_cast<size_t>(width) * 3U;
		if (codepoint >= 0x110000U || width > 64U || static_cast<size_t>(end - ptr) < recordSize)
			return false;
		if (!zhFontPointers.empty() && codepoint <= previous)
			return false;

		if (!haveRange || codepoint != previous + 1U)
		{
			if (haveRange)
				zhFontRanges.push_back({ rangeFirst, previous, rangeOffset });
			rangeFirst = codepoint;
			rangeOffset = zhFontPointers.size();
			haveRange = true;
		}
		zhFontPointers.push_back(static_cast<size_t>(ptr + 3 - begin));
		previous = codepoint;
		ptr += recordSize;
	}
	if (haveRange)
		zhFontRanges.push_back({ rangeFirst, previous, rangeOffset });
	zhFontInitialised = true;
	return true;
}
}

DecodedUtf8 DecodeUtf8(const char *text, size_t remaining)
{
	if (!text || remaining == 0)
		return { 0, 0 };
	const unsigned char b0 = static_cast<unsigned char>(text[0]);
	if (b0 < 0x80U)
		return { b0, 1 };

	size_t length;
	uint32_t codepoint;
	if (b0 >= 0xC2U && b0 <= 0xDFU)
	{
		length = 2;
		codepoint = b0 & 0x1FU;
	}
	else if (b0 >= 0xE0U && b0 <= 0xEFU)
	{
		length = 3;
		codepoint = b0 & 0x0FU;
	}
	else if (b0 >= 0xF0U && b0 <= 0xF4U)
	{
		length = 4;
		codepoint = b0 & 0x07U;
	}
	else
	{
		return { b0, 1 };
	}

	if (remaining != static_cast<size_t>(-1) && remaining < length)
		return { b0, 1 };
	for (size_t i = 1; i < length; ++i)
	{
		if (remaining == static_cast<size_t>(-1) && text[i] == '\0')
			return { b0, 1 };
		const unsigned char next = static_cast<unsigned char>(text[i]);
		if ((next & 0xC0U) != 0x80U)
			return { b0, 1 };
		codepoint = (codepoint << 6) | (next & 0x3FU);
	}

	if ((length == 3 && codepoint < 0x800U) ||
		(length == 4 && codepoint < 0x10000U) ||
		(codepoint >= 0xD800U && codepoint <= 0xDFFFU) ||
		codepoint > 0x10FFFFU)
		return { b0, 1 };
	return { codepoint, length };
}

ZhFontReader::ZhFontReader(const unsigned char *glyph):
	pointer(glyph + 1),
	width(*glyph),
	pixels(0),
	data(0)
{
}

const unsigned char *ZhFontReader::LookupChar(uint32_t codepoint)
{
	if (!InitZhFontData())
		throw std::runtime_error("Simplified Chinese font data is corrupt");
	for (const auto &range: zhFontRanges)
	{
		if (codepoint < range.first)
			break;
		if (codepoint <= range.last)
			return reinterpret_cast<const unsigned char *>(zhFontData.data()) +
				zhFontPointers[range.offset + codepoint - range.first];
	}
	if (codepoint != 0xFFFDU)
		return LookupChar(0xFFFDU);
	return reinterpret_cast<const unsigned char *>(zhFontData.data()) + zhFontPointers.front();
}

ZhFontReader::ZhFontReader(uint32_t codepoint):
	ZhFontReader(LookupChar(codepoint))
{
}

int ZhFontReader::GetWidth() const
{
	return width;
}

int ZhFontReader::NextPixel()
{
	if (!pixels)
	{
		data = *(pointer++);
		pixels = 4;
	}
	int value = data & 0x3;
	data >>= 2;
	--pixels;
	return value;
}
