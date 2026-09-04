#include "graphics/ZhFontReader.h"

#include <iostream>

int main()
{
	const char *text = u8"中文";
	auto first = DecodeUtf8(text);
	auto second = DecodeUtf8(text + first.length);
	if (first.codepoint != 0x4E2DU || second.codepoint != 0x6587U || first.length != 3 || second.length != 3)
		return 1;

	for (uint32_t codepoint: { first.codepoint, second.codepoint, 0xFF0CU })
	{
		ZhFontReader glyph(codepoint);
		int ink = 0;
		for (int i = 0; i < glyph.GetWidth() * ZH_FONT_H; ++i)
			ink += glyph.NextPixel();
		if (glyph.GetWidth() < 8 || ink == 0)
			return 2;
		std::cout << std::hex << codepoint << ':' << std::dec << glyph.GetWidth() << ':' << ink << '\n';
	}
	return 0;
}
