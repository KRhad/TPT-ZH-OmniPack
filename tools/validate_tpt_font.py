#!/usr/bin/env python3
"""Validate and inspect the TPT 12px bitmap font used by FontReader.

The TPT container stores a little-endian three-byte Unicode codepoint, a
width byte and exactly ``width * 3`` bytes of 2bpp bitmap data.  Pixels are
row-major and low-bit-first within each byte, matching FontReader::NextPixel.
"""

from __future__ import annotations

import argparse
import bz2
import csv
import json
import struct
import sys
import zlib
from pathlib import Path
from typing import Iterable, Sequence

from build_release_font import (
    FONT_HEIGHT,
    convert_unifont_glyph,
    parse_fusion_bdf,
    parse_unifont,
    unpack_tpt_glyph,
)


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


def periodic_codepoints(path: Path | None) -> set[int]:
    if path is None or not path.is_file():
        return set()
    with path.open("r", encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != 118 or any(not row.get("zh_name") for row in rows):
        raise ValueError(f"invalid periodic element source map: {path}")
    return {ord(character) for row in rows for character in row["zh_name"] if ord(character) >= 0x20}


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


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def save_glyph_png(path: Path, codepoint: int, width: int, matrix: Sequence[Sequence[int]], scale: int = 16) -> None:
    """Write a dependency-free grayscale PNG with nearest-neighbour scaling."""
    image_width = width * scale
    image_height = FONT_HEIGHT * scale
    scanlines = bytearray()
    for row in matrix:
        expanded = bytes(value * 85 for value in row for _ in range(scale))
        for _ in range(scale):
            scanlines.append(0)  # PNG filter: None
            scanlines.extend(expanded)
    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", image_width, image_height, 8, 0, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(bytes(scanlines), 9))
    png += png_chunk(b"IEND", b"")
    path.write_bytes(png)


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


def validate(
    font: Path,
    language_paths: list[Path],
    output: Path | None,
    unifont: Path | None,
    fusion_bdf: Path | None,
    periodic_map: Path | None = None,
) -> dict[str, object]:
    glyphs = parse_font(font)
    required = catalog_codepoints(language_paths)
    periodic_required = periodic_codepoints(periodic_map)
    required.update(periodic_required)
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
        missing_text = ",".join(f"U+{codepoint:04X}" for codepoint in missing[:32])
        raise ValueError(
            "validation failed: "
            f"missing={len(missing)} invalid_width={len(invalid_width)} "
            f"empty={len(empty)} roundtrip={len(roundtrip_failures)} "
            f"missing_codepoints={missing_text or 'none'}"
        )

    known_missing = [character for character in KNOWN_TEXT if ord(character) not in glyphs]
    known_empty = [character for character in KNOWN_TEXT if not any(pixel for row in glyph_pixels(*glyphs[ord(character)]) for pixel in row)]
    if known_missing or known_empty:
        raise ValueError("known Chinese glyphs invalid: missing=" + "".join(known_missing) + " empty=" + "".join(known_empty))

    source_decode_test = False
    if unifont is not None:
        probe_source = parse_unifont(unifont, set(PROBE_CODEPOINTS))
        source_missing = sorted(set(PROBE_CODEPOINTS) - set(probe_source))
        roundtrip_failed = []
        for codepoint, source_glyph in probe_source.items():
            width, bitmap = convert_unifont_glyph(*source_glyph)
            if pack_pixels([pixel for row in glyph_pixels(width, bitmap) for pixel in row]) != bitmap:
                roundtrip_failed.append(codepoint)
        if source_missing or roundtrip_failed:
            raise ValueError(
                "Unifont source decode/roundtrip mismatch: "
                f"missing={','.join(f'U+{codepoint:04X}' for codepoint in source_missing)} "
                f"roundtrip={','.join(f'U+{codepoint:04X}' for codepoint in roundtrip_failed)}"
            )
        source_decode_test = True
    fusion_source_decode_test = False
    if fusion_bdf is not None:
        probe_source = parse_fusion_bdf(fusion_bdf, set(PROBE_CODEPOINTS))
        source_missing = sorted(set(PROBE_CODEPOINTS) - set(probe_source))
        mismatched = [
            codepoint for codepoint, source_glyph in probe_source.items()
            if glyphs.get(codepoint) != source_glyph
        ]
        if source_missing or mismatched:
            raise ValueError(
                "Fusion BDF conversion mismatch: "
                f"missing={','.join(f'U+{codepoint:04X}' for codepoint in source_missing)} "
                f"mismatched={','.join(f'U+{codepoint:04X}' for codepoint in mismatched)}"
            )
        fusion_source_decode_test = True
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
        "periodic_required_codepoints": len(periodic_required),
        "font_glyph_coverage_valid": True,
        "font_pack_roundtrip_test": True,
        "unifont_source_decode_test": source_decode_test,
        "fusion_bdf_source_decode_test": fusion_source_decode_test,
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
    parser.add_argument("--fusion-bdf", type=Path)
    parser.add_argument("--periodic-map", type=Path, default=Path("docs/PERIODIC_ELEMENT_SOURCE_MAP.csv"))
    args = parser.parse_args()
    try:
        result = validate(
            args.font,
            sorted(args.language_dir.glob("*.json")),
            args.output,
            args.unifont,
            args.fusion_bdf,
            args.periodic_map,
        )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"validate-tpt-font: ERROR {error}", file=sys.stderr)
        return 1
    print("validate-tpt-font: PASS " + " ".join(f"{key}={value}" for key, value in result.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
