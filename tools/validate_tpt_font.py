#!/usr/bin/env python3
"""Validate and inspect the TPT 12px bitmap font used by FontReader.

The TPT container stores a little-endian three-byte Unicode codepoint, a
width byte and exactly ``width * 3`` bytes of 2bpp bitmap data.  Pixels are
row-major and low-bit-first within each byte, matching FontReader::NextPixel.
"""

from __future__ import annotations

import argparse
import bz2
import json
import sys
from pathlib import Path
from typing import Iterable, Sequence

try:
    from PIL import Image
except ImportError:  # pragma: no cover - explicitly diagnosed for release tooling
    Image = None

from build_release_font import FONT_HEIGHT, convert_unifont_glyph, parse_unifont, unpack_tpt_glyph


KNOWN_TEXT = "简体中文工业冶金局部生态高级化学受控核设置保存加载只读取消"
PROBE_CODEPOINTS = (0x4E2D, 0x6587, 0x7B80, 0x4F53, 0x5DE5, 0x4E1A)
INTENTIONALLY_BLANK = { 0x0020 }


def parse_font(path: Path) -> dict[int, tuple[int, bytes]]:
    raw = bz2.decompress(path.read_bytes())
    glyphs: dict[int, tuple[int, bytes]] = {}
    position = 0
    previous = -1
    while position < len(raw):
        if position + 4 > len(raw):
            raise ValueError(f"truncated header at byte {position}")
        codepoint = int.from_bytes(raw[position : position + 3], "little")
        width = raw[position + 3]
        end = position + 4 + width * 3
        if codepoint > 0x10FFFF:
            raise ValueError(f"invalid codepoint U+{codepoint:06X}")
        if codepoint <= previous:
            raise ValueError(f"codepoints are not strictly increasing at U+{codepoint:04X}")
        # The upstream font intentionally contains zero-width control and
        # combining glyphs. They have no bitmap bytes and are valid as long
        # as their record is exactly aligned.
        if width > 64:
            raise ValueError(f"invalid width {width} for U+{codepoint:04X}")
        if end > len(raw):
            raise ValueError(f"glyph U+{codepoint:04X} crosses EOF")
        glyphs[codepoint] = (width, raw[position + 4 : end])
        previous = codepoint
        position = end
    if position != len(raw):
        raise ValueError("font does not end on a glyph boundary")
    return glyphs


def catalog_codepoints(paths: Iterable[Path]) -> set[int]:
    codepoints: set[int] = set()
    for path in paths:
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data, dict) or not all(isinstance(value, str) for value in data.values()):
            raise ValueError(f"invalid language catalog {path}")
        for value in data.values():
            codepoints.update(ord(character) for character in value if ord(character) >= 0x20)
    return codepoints


def glyph_pixels(width: int, bitmap: bytes) -> tuple[tuple[int, ...], ...]:
    if width == 0:
        return tuple(() for _ in range(FONT_HEIGHT))
    return unpack_tpt_glyph(width, bitmap)


def pack_pixels(pixels: Sequence[int]) -> bytes:
    packed = bytearray()
    for index in range(0, len(pixels), 4):
        value = 0
        for offset, pixel in enumerate(pixels[index : index + 4]):
            value |= (pixel & 0x3) << (offset * 2)
        packed.append(value)
    return bytes(packed)


def save_glyph_png(path: Path, codepoint: int, width: int, matrix: Sequence[Sequence[int]], scale: int = 16) -> None:
    if Image is None:
        raise RuntimeError("Pillow is required to export glyph PNGs")
    image = Image.new("L", (width, FONT_HEIGHT), 0)
    image.putdata([value * 85 for row in matrix for value in row])
    image.resize((width * scale, FONT_HEIGHT * scale), Image.Resampling.NEAREST).save(path)


def degradation(width: int, matrix: Sequence[Sequence[int]]) -> list[str]:
    active = [(x, y) for y, row in enumerate(matrix) for x, value in enumerate(row) if value]
    if not active:
        return ["empty"]
    xs, ys = [point[0] for point in active], [point[1] for point in active]
    bbox_w, bbox_h = max(xs) - min(xs) + 1, max(ys) - min(ys) + 1
    warnings = []
    if bbox_w == 1:
        warnings.append("single_vertical_line")
    if bbox_h == 1:
        warnings.append("single_horizontal_line")
    if len(active) <= 3:
        warnings.append("too_few_pixels")
    if min(xs) > width // 2 or max(xs) < width // 2:
        warnings.append("pixels_concentrated_on_one_side")
    return warnings


def validate(font: Path, language_paths: list[Path], output: Path | None, unifont: Path | None) -> dict[str, object]:
    glyphs = parse_font(font)
    required = catalog_codepoints(language_paths)
    missing = sorted(required - set(glyphs))
    invalid_width = []
    empty = []
    roundtrip_failures = []
    fingerprint_to_codepoints: dict[bytes, list[int]] = {}
    warnings: dict[str, list[str]] = {}
    for codepoint, (width, bitmap) in glyphs.items():
        matrix = glyph_pixels(width, bitmap)
        if len(bitmap) != width * 3:
            invalid_width.append(codepoint)
        if pack_pixels([pixel for row in matrix for pixel in row]) != bitmap:
            roundtrip_failures.append(codepoint)
        if codepoint in required and codepoint not in INTENTIONALLY_BLANK and width > 0 and not any(pixel for row in matrix for pixel in row):
            empty.append(codepoint)
        if codepoint >= 0x4E00 and codepoint <= 0x9FFF:
            fingerprint_to_codepoints.setdefault(bitmap, []).append(codepoint)
            issue = degradation(width, matrix)
            if issue:
                warnings[f"U+{codepoint:04X}"] = issue

    replacement = glyphs.get(0xFFFD)
    if replacement is None or not any(pixel for row in glyph_pixels(*replacement) for pixel in row):
        raise ValueError("replacement glyph U+FFFD is missing or empty")
    if missing or invalid_width or empty or roundtrip_failures:
        raise ValueError(
            "validation failed: "
            f"missing={len(missing)} invalid_width={len(invalid_width)} "
            f"empty={len(empty)} roundtrip={len(roundtrip_failures)}"
        )

    known_missing = [character for character in KNOWN_TEXT if ord(character) not in glyphs]
    known_empty = [character for character in KNOWN_TEXT if not any(pixel for row in glyph_pixels(*glyphs[ord(character)]) for pixel in row)]
    if known_missing or known_empty:
        raise ValueError("known Chinese glyphs invalid: missing=" + "".join(known_missing) + " empty=" + "".join(known_empty))

    source_decode_test = False
    if unifont is not None:
        probe_source = parse_unifont(unifont, set(PROBE_CODEPOINTS))
        source_missing = sorted(set(PROBE_CODEPOINTS) - set(probe_source))
        mismatched = [
            codepoint for codepoint, source_glyph in probe_source.items()
            if glyphs.get(codepoint) != convert_unifont_glyph(*source_glyph)
        ]
        if source_missing or mismatched:
            raise ValueError(
                "Unifont conversion mismatch: "
                f"missing={','.join(f'U+{codepoint:04X}' for codepoint in source_missing)} "
                f"mismatched={','.join(f'U+{codepoint:04X}' for codepoint in mismatched)}"
            )
        source_decode_test = True
    if output:
        output.mkdir(parents=True, exist_ok=True)
        for codepoint in sorted({ord(character) for character in KNOWN_TEXT} | set(PROBE_CODEPOINTS)):
            width, bitmap = glyphs[codepoint]
            save_glyph_png(output / f"U+{codepoint:04X}.png", codepoint, width, glyph_pixels(width, bitmap))
    duplicate_cjk = sum(1 for values in fingerprint_to_codepoints.values() if len(values) > 1)
    return {
        "font_container_valid": True,
        "glyphs": len(glyphs),
        "required_codepoints": len(required),
        "font_glyph_coverage_valid": True,
        "font_pack_roundtrip_test": True,
        "unifont_source_decode_test": source_decode_test,
        "zh_known_glyph_test": True,
        "cjk_duplicate_bitmaps": duplicate_cjk,
        "cjk_degradation_warnings": len(warnings),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", type=Path, default=Path("resources/font.bz2"))
    parser.add_argument("--language-dir", type=Path, default=Path("src/lang"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--unifont", type=Path)
    args = parser.parse_args()
    try:
        result = validate(args.font, sorted(args.language_dir.glob("*.json")), args.output, args.unifont)
    except (OSError, ValueError, json.JSONDecodeError, RuntimeError) as error:
        print(f"validate-tpt-font: ERROR {error}", file=sys.stderr)
        return 1
    print("validate-tpt-font: PASS " + " ".join(f"{key}={value}" for key, value in result.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
