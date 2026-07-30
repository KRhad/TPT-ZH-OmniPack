#!/usr/bin/env python3
"""Build the embedded TPT bitmap font from pinned, redistributable sources.

The upstream TPT font supplies the UI glyphs.  GNU Unifont supplies only the
glyphs needed by the embedded language catalogs that upstream does not have.
The output format is consumed by ``src/graphics/FontReader.cpp``.
"""

from __future__ import annotations

import argparse
import bz2
import gzip
import hashlib
import json
from pathlib import Path
import sys
from typing import Iterable, Sequence


FONT_HEIGHT = 12
UPSTREAM_FONT = "resources/third_party/tpt-upstream-font-100.0.399.bz2"
UNIFONT_HEX = "resources/third_party/unifont_all-16.0.03.hex.gz"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def parse_tpt_font(path: Path) -> dict[int, tuple[int, bytes]]:
    raw = bz2.decompress(path.read_bytes())
    glyphs: dict[int, tuple[int, bytes]] = {}
    position = 0
    while position < len(raw):
        if position + 4 > len(raw):
            raise ValueError(f"truncated glyph header in {path}")
        codepoint = int.from_bytes(raw[position : position + 3], "little")
        width = raw[position + 3]
        size = width * 3
        position += 4
        if width > 64 or position + size > len(raw):
            raise ValueError(f"invalid glyph U+{codepoint:04X} in {path}")
        if codepoint in glyphs:
            raise ValueError(f"duplicate glyph U+{codepoint:04X} in {path}")
        glyphs[codepoint] = (width, raw[position : position + size])
        position += size
    return glyphs


def required_codepoints(language_paths: Iterable[Path]) -> set[int]:
    codepoints: set[int] = set()
    for path in language_paths:
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data, dict) or not all(
            isinstance(key, str) and isinstance(value, str)
            for key, value in data.items()
        ):
            raise ValueError(f"language catalog is not a flat string object: {path}")
        for value in data.values():
            codepoints.update(ord(character) for character in value if ord(character) >= 0x20)
    return codepoints


def parse_unifont(path: Path, required: set[int]) -> dict[int, tuple[int, tuple[int, ...]]]:
    glyphs: dict[int, tuple[int, tuple[int, ...]]] = {}
    with gzip.open(path, "rt", encoding="ascii") as stream:
        for line in stream:
            line = line.strip()
            if not line or ":" not in line:
                continue
            codepoint_text, bitmap_text = line.split(":", 1)
            try:
                codepoint = int(codepoint_text, 16)
            except ValueError:
                continue
            if codepoint not in required:
                continue
            if len(bitmap_text) == 32:
                width = 8
            elif len(bitmap_text) == 64:
                width = 16
            else:
                continue
            rows = tuple(int(bitmap_text[index : index + width // 4], 16) for index in range(0, len(bitmap_text), width // 4))
            if len(rows) != 16:
                raise ValueError(f"invalid GNU Unifont glyph U+{codepoint:04X}")
            glyphs[codepoint] = (width, rows)
    return glyphs


def convert_unifont_glyph(width: int, rows: tuple[int, ...]) -> tuple[int, bytes]:
    # A 16x16 1-bit bitmap is resampled to TPT's 12-pixel font height.  The
    # width follows the same 3:4 ratio so CJK glyphs stay square.
    output_width = max(1, round(width * FONT_HEIGHT / 16))
    pixels: list[int] = []
    for output_y in range(FONT_HEIGHT):
        source_y = min(15, (output_y * 16) // FONT_HEIGHT)
        row = rows[source_y]
        for output_x in range(output_width):
            source_x = min(width - 1, (output_x * width) // output_width)
            pixels.append(3 if row & (1 << (width - 1 - source_x)) else 0)
    packed = bytearray()
    for index in range(0, len(pixels), 4):
        value = 0
        for pixel in pixels[index : index + 4]:
            value = (value << 2) | pixel
        value <<= 2 * (4 - len(pixels[index : index + 4]))
        packed.append(value)
    return output_width, bytes(packed)


def encode_tpt_font(glyphs: dict[int, tuple[int, bytes]]) -> bytes:
    encoded = bytearray()
    for codepoint, (width, bitmap) in sorted(glyphs.items()):
        if len(bitmap) != width * 3:
            raise ValueError(f"invalid encoded glyph U+{codepoint:04X}")
        encoded.extend(codepoint.to_bytes(3, "little"))
        encoded.append(width)
        encoded.extend(bitmap)
    return bz2.compress(bytes(encoded), compresslevel=9)


def build_font(source_root: Path, output: Path) -> dict[str, int | str]:
    source_root = source_root.resolve()
    upstream_path = source_root / UPSTREAM_FONT
    unifont_path = source_root / UNIFONT_HEX
    language_paths = sorted((source_root / "src/lang").glob("*.json"))
    if not language_paths:
        raise ValueError("no embedded language catalogs found")
    for path in (upstream_path, unifont_path):
        if not path.is_file():
            raise ValueError(f"required font input is missing: {path}")

    glyphs = parse_tpt_font(upstream_path)
    required = required_codepoints(language_paths)
    missing = required - set(glyphs)
    unifont = parse_unifont(unifont_path, missing)
    unresolved = sorted(missing - set(unifont))
    if unresolved:
        rendered = ", ".join(f"U+{codepoint:04X}" for codepoint in unresolved[:12])
        suffix = "" if len(unresolved) <= 12 else f" (+{len(unresolved) - 12})"
        raise ValueError(f"GNU Unifont does not cover embedded language glyphs: {rendered}{suffix}")
    for codepoint in missing:
        glyphs[codepoint] = convert_unifont_glyph(*unifont[codepoint])

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(encode_tpt_font(glyphs))
    return {
        "base_glyphs": len(glyphs) - len(missing),
        "added_unifont_glyphs": len(missing),
        "glyphs": len(glyphs),
        "required_glyphs": len(required),
        "output_bytes": output.stat().st_size,
        "output_sha256": sha256(output),
        "upstream_font_sha256": sha256(upstream_path),
        "unifont_sha256": sha256(unifont_path),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, default=Path("resources/font.bz2"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    source_root = args.source_root.resolve()
    output = args.output if args.output.is_absolute() else source_root / args.output
    try:
        result = build_font(source_root, output)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"release-font: ERROR {exc}", file=sys.stderr)
        return 1
    print("release-font: PASS " + " ".join(f"{key}={value}" for key, value in result.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
