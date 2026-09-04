#pragma once

#include <cstddef>
#include <cstdint>

constexpr int ZH_FONT_H = 12;

struct DecodedUtf8
{
	uint32_t codepoint;
	size_t length;
};

// Invalid UTF-8 is deliberately returned as one legacy byte. The original
// Powder Toy font uses bytes 0x80-0xFF for icons, so they must remain usable.
DecodedUtf8 DecodeUtf8(const char *text, size_t remaining = static_cast<size_t>(-1));

class ZhFontReader
{
	const unsigned char *pointer;
	int width;
	int pixels;
	int data;

	explicit ZhFontReader(const unsigned char *pointer);
	static const unsigned char *LookupChar(uint32_t codepoint);

public:
	explicit ZhFontReader(uint32_t codepoint);
	int GetWidth() const;
	int NextPixel();
};
