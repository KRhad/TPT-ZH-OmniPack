from __future__ import annotations

from pathlib import Path
import sys
import unittest


TOOLS = Path(__file__).resolve().parents[1]
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from build_release_font import (
    FONT_HEIGHT,
    convert_bdf_glyph,
    convert_unifont_glyph,
    pack_tpt_pixels,
    parse_fusion_bdf,
    parse_tpt_font,
    player_text_codepoints,
    unpack_tpt_glyph,
)
from validate_tpt_font import save_glyph_png


class ReleaseFontTests(unittest.TestCase):
    def test_player_visible_material_text_is_covered_by_release_font(self) -> None:
        root = Path(__file__).resolve().parents[2]
        required, chinese_required = player_text_codepoints(root)
        glyphs = parse_tpt_font(root / "resources/font.bz2")
        self.assertTrue({ord(character) for character in "玩芯焊"} <= chinese_required)
        self.assertEqual(required - set(glyphs), set())

    def test_native_bdf_glyph_maps_directly_without_scaling(self) -> None:
        width, bitmap = convert_bdf_glyph(
            4,
            (2, 2, 1, 0),
            ("80", "40"),
        )
        matrix = unpack_tpt_glyph(width, bitmap)
        self.assertEqual(width, 4)
        self.assertEqual(matrix[8], (0, 3, 0, 0))
        self.assertEqual(matrix[9], (0, 0, 3, 0))
        active = {(x, y) for y, row in enumerate(matrix) for x, pixel in enumerate(row) if pixel}
        self.assertEqual(active, {(1, 8), (2, 9)})

    def test_pinned_fusion_bdf_has_fixed_native_chinese_matrices(self) -> None:
        expected_rows = {
            0x4E2D: (0x000, 0x040, 0x040, 0xFFE, 0x842, 0x842, 0x842, 0xFFE, 0x040, 0x040, 0x040, 0x040),
            0x6587: (0x000, 0x080, 0x040, 0xFFE, 0x208, 0x208, 0x110, 0x110, 0x0A0, 0x040, 0x1B0, 0xE0E),
            0x7B80: (0x000, 0x420, 0x7BE, 0x948, 0x5FC, 0x004, 0x5F4, 0x514, 0x5F4, 0x514, 0x5F4, 0x40C),
            0x4F53: (0x000, 0x220, 0x220, 0x5FE, 0x420, 0xC70, 0x4A8, 0x4A8, 0x524, 0x6FA, 0x420, 0x420),
            0x5DE5: (0x000, 0x7FC, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0x040, 0xFFE),
            0x4E1A: (0x000, 0x110, 0x110, 0x110, 0x912, 0x912, 0x514, 0x514, 0x110, 0x110, 0x110, 0xFFE),
        }
        root = Path(__file__).resolve().parents[2]
        source = root / "resources/third_party/fusion-pixel-12px-monospaced-zh_hans-v2026.07.20.bdf"
        glyphs = parse_fusion_bdf(source, set(expected_rows))
        self.assertEqual(set(glyphs), set(expected_rows))
        for codepoint, expected in expected_rows.items():
            width, bitmap = glyphs[codepoint]
            matrix = unpack_tpt_glyph(width, bitmap)
            actual = tuple(sum((1 << (width - 1 - x)) for x, pixel in enumerate(row) if pixel) for row in matrix)
            self.assertEqual(width, 12)
            self.assertEqual(actual, expected, f"U+{codepoint:04X}")
            self.assertTrue(all(pixel in (0, 3) for row in matrix for pixel in row))

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

    def test_glyph_png_writer_has_no_external_image_dependency(self) -> None:
        import tempfile

        matrix = tuple(tuple(3 if x == y else 0 for x in range(12)) for y in range(FONT_HEIGHT))
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "glyph.png"
            save_glyph_png(output, 0x4E2D, 12, matrix, scale=2)
            data = output.read_bytes()
        self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
        self.assertIn(b"IHDR", data)
        self.assertIn(b"IDAT", data)


if __name__ == "__main__":
    unittest.main()
