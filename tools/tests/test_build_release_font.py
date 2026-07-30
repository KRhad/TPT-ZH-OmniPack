from __future__ import annotations

from pathlib import Path
import sys
import unittest


TOOLS = Path(__file__).resolve().parents[1]
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from build_release_font import FONT_HEIGHT, convert_unifont_glyph, pack_tpt_pixels, unpack_tpt_glyph


class ReleaseFontTests(unittest.TestCase):
    def test_tpt_pixels_are_low_bit_first_for_font_reader(self) -> None:
        self.assertEqual(pack_tpt_pixels([3, 2, 1, 0]), bytes([0x1B]))

    def test_convert_roundtrips_through_font_reader_order(self) -> None:
        rows = tuple(0x8100 if index in (0, 15) else 0x0000 for index in range(16))
        width, bitmap = convert_unifont_glyph(16, rows)
        self.assertEqual(width, 12)
        self.assertEqual(len(bitmap), width * 3)
        matrix = unpack_tpt_glyph(width, bitmap)
        self.assertEqual(len(matrix), FONT_HEIGHT)
        self.assertTrue(any(pixel for row in matrix for pixel in row))

    def test_area_conversion_preserves_a_stroke_on_source_row_seven(self) -> None:
        # The rejected point sampler skipped source row 7, turning U+4E00
        # into an empty glyph. Coverage conversion must retain this stroke.
        rows = tuple(0x7FFE if index == 7 else 0x0000 for index in range(16))
        width, bitmap = convert_unifont_glyph(16, rows)
        matrix = unpack_tpt_glyph(width, bitmap)
        self.assertEqual(width, 12)
        self.assertTrue(any(pixel for row in matrix for pixel in row))


if __name__ == "__main__":
    unittest.main()
