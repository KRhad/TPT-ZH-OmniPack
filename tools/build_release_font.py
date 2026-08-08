#!/usr/bin/env python3
"""Build the embedded TPT bitmap font from pinned, redistributable sources.

The upstream TPT font supplies UI, icon and Latin glyphs. Fusion Pixel Font's
native 12px Simplified Chinese BDF supplies missing Chinese glyphs without
resampling. GNU Unifont remains the fallback for other embedded language
catalogs. The output format is consumed by ``src/graphics/FontReader.cpp``.
"""

from __future__ import annotations

import argparse
import bz2
import csv
import gzip
import hashlib
import json
from pathlib import Path
import sys
from typing import Iterable, Sequence


FONT_HEIGHT = 12
BDF_ASCENT = 10
UPSTREAM_FONT = "resources/third_party/tpt-upstream-font-100.0.399.bz2"
UNIFONT_HEX = "resources/third_party/unifont_all-16.0.03.hex.gz"
FUSION_BDF = "resources/third_party/fusion-pixel-12px-monospaced-zh_hans-v2026.07.20.bdf"
FUSION_LICENSES = (
    "resources/third_party/FUSION_PIXEL_FONT_OFL-1.1.txt",
    "resources/third_party/FUSION_PIXEL_FONT_ARK_PIXEL_OFL-1.1.txt",
    "resources/third_party/FUSION_PIXEL_FONT_CUBIC_11_OFL-1.1.txt",
    "resources/third_party/FUSION_PIXEL_FONT_GALMURI_OFL-1.1.txt",
)
PINNED_INPUT_SHA256 = {
    UPSTREAM_FONT: "EA86995CC429146F9869C172DC6DAE63D9B6E8BBFC7AC7D07D0CF30FB07D17F7",
    UNIFONT_HEX: "23AB31CA87C6614B97928A39DC0C15BC2AAA5B6F130ADF9E1DF481250F73BAAF",
    FUSION_BDF: "8E4A12E821EFAD608BCB464D685CE50C70693F85A1E95DEAD9575E6CECAFFFC7",
    "resources/third_party/GNU_UNIFONT_COPYING.txt":
        "1E74CB82BF476843E97C2596297B04219B1A7E51F7238944A8C031CB9401FA87",
    FUSION_LICENSES[0]: "BC518CF64B8032C07690F33CC270C35C179255A6AC8EFA7C165EBAE7E8F76A63",
    FUSION_LICENSES[1]: "3AB41567E68E3988BA1EF16DD2644ECA95CA5648EA12E7D46E6287FC0BBE5AEE",
    FUSION_LICENSES[2]: "2B6E5938E5CFFA0B9E183BD05F8C363E174E7EBED1A0556E2855FD1707FA2188",
    FUSION_LICENSES[3]: "86A3EE9495F942F0243F18C103DA9FACA27ADB88142613EDB8BB852E56C892C1",
}
FUSION_XLFD = "-TakWolf-Fusion Pixel 12px Mono zh_hans-Regular-R-Normal-Sans Serif-12-120-75-75-M-118-ISO10646-1"

PLAYER_TEXT_CSV_FIELDS = (
    (
        "docs/ELEMENT_REGISTRY.csv",
        (
            "display_code",
            "english_name",
            "chinese_name",
            "english_description",
            "chinese_description",
        ),
        ("chinese_name", "chinese_description"),
    ),
    (
        "docs/ELEMENT_CONTENT.csv",
        (
            "recipe_en",
            "recipe_zh",
            "production_en",
            "production_zh",
            "use_en",
            "use_zh",
            "hazard_en",
            "hazard_zh",
        ),
        ("recipe_zh", "production_zh", "use_zh", "hazard_zh"),
    ),
    (
        "docs/OFFICIAL_ELEMENT_DESCRIPTIONS.csv",
        ("english_description", "chinese_description"),
        ("chinese_description",),
    ),
    (
        "docs/PERIODIC_CONTENT_LINKS.csv",
        ("formula", "zh_name", "en_name"),
        ("zh_name",),
    ),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def verify_pinned_inputs(source_root: Path) -> None:
    for relative, expected in PINNED_INPUT_SHA256.items():
        path = source_root / relative
        if not path.is_file():
            raise ValueError(f"required pinned font input is missing: {path}")
        actual = sha256(path)
        if actual != expected:
            raise ValueError(
                f"pinned font input hash mismatch for {relative}: "
                f"expected {expected}, got {actual}"
            )


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


def periodic_codepoints(path: Path) -> set[int]:
    if not path.is_file():
        return set()
    with path.open("r", encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != 118 or any(not row.get("zh_name") for row in rows):
        raise ValueError(f"invalid periodic element source map: {path}")
    return {ord(character) for row in rows for character in row["zh_name"] if ord(character) >= 0x20}


def player_text_codepoints(source_root: Path) -> tuple[set[int], set[int]]:
    """Return all and Chinese codepoints embedded in player-facing material text."""
    required: set[int] = set()
    chinese_required: set[int] = set()
    for relative, fields, chinese_fields in PLAYER_TEXT_CSV_FIELDS:
        path = source_root / relative
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            reader = csv.DictReader(stream)
            fieldnames = tuple(reader.fieldnames or ())
            missing_fields = sorted(set(fields) - set(fieldnames))
            if missing_fields:
                raise ValueError(
                    f"player text CSV {path} is missing fields: "
                    + ", ".join(missing_fields)
                )
            for row in reader:
                for field in fields:
                    codepoints = {
                        ord(character)
                        for character in (row.get(field) or "")
                        if ord(character) >= 0x20
                    }
                    required.update(codepoints)
                    if field in chinese_fields:
                        chinese_required.update(codepoints)
    return required, chinese_required


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


def convert_bdf_glyph(
    advance_width: int,
    bounding_box: tuple[int, int, int, int],
    bitmap_rows: Sequence[str],
) -> tuple[int, bytes]:
    """Map one BDF glyph into TPT's 12-row cell without scaling."""
    glyph_width, glyph_height, offset_x, offset_y = bounding_box
    if not 1 <= advance_width <= 64:
        raise ValueError(f"invalid BDF advance width {advance_width}")
    if glyph_width < 0 or glyph_height < 0 or len(bitmap_rows) != glyph_height:
        raise ValueError("invalid BDF glyph dimensions")
    bytes_per_row = (glyph_width + 7) // 8
    pixels = [[0 for _ in range(advance_width)] for _ in range(FONT_HEIGHT)]
    for source_y, bitmap_text in enumerate(bitmap_rows):
        if len(bitmap_text) != bytes_per_row * 2:
            raise ValueError("BDF bitmap row length does not match BBX width")
        try:
            bits = int(bitmap_text, 16) if bitmap_text else 0
        except ValueError as exc:
            raise ValueError(f"invalid BDF bitmap row {bitmap_text!r}") from exc
        padding = bytes_per_row * 8 - glyph_width
        if padding and bits & ((1 << padding) - 1):
            raise ValueError("BDF bitmap has nonzero row-padding bits")
        glyph_y = offset_y + glyph_height - 1 - source_y
        target_y = BDF_ASCENT - 1 - glyph_y
        for source_x in range(glyph_width):
            if not bits & (1 << (bytes_per_row * 8 - 1 - source_x)):
                continue
            target_x = offset_x + source_x
            if not (0 <= target_x < advance_width and 0 <= target_y < FONT_HEIGHT):
                raise ValueError("BDF glyph has a lit pixel outside its advance cell")
            pixels[target_y][target_x] = 3
    return advance_width, pack_tpt_pixels([pixel for row in pixels for pixel in row])


def _parse_bdf_glyph(block: Sequence[str]) -> tuple[int, int, tuple[int, int, int, int], tuple[str, ...]]:
    encoding: int | None = None
    advance: int | None = None
    bounding_box: tuple[int, int, int, int] | None = None
    bitmap_rows: list[str] = []
    in_bitmap = False
    for line in block:
        if line.startswith("ENCODING "):
            fields = line.split()
            if len(fields) != 2:
                raise ValueError("invalid BDF ENCODING record")
            encoding = int(fields[1])
        elif line.startswith("DWIDTH "):
            fields = line.split()
            if len(fields) != 3 or fields[2] != "0":
                raise ValueError("unsupported BDF DWIDTH record")
            advance = int(fields[1])
        elif line.startswith("BBX "):
            fields = line.split()
            if len(fields) != 5:
                raise ValueError("invalid BDF BBX record")
            bounding_box = tuple(int(value) for value in fields[1:])
        elif line == "BITMAP":
            if in_bitmap:
                raise ValueError("duplicate BDF BITMAP record")
            in_bitmap = True
        elif in_bitmap and line != "ENDCHAR":
            bitmap_rows.append(line)
    if encoding is None or advance is None or bounding_box is None or not in_bitmap:
        raise ValueError("incomplete BDF glyph record")
    return encoding, advance, bounding_box, tuple(bitmap_rows)


def parse_fusion_bdf(path: Path, required: set[int]) -> dict[int, tuple[int, bytes]]:
    """Parse and validate the pinned Fusion Pixel Font BDF."""
    font_name: str | None = None
    size: tuple[int, int, int] | None = None
    font_box: tuple[int, int, int, int] | None = None
    expected_characters: int | None = None
    properties: dict[str, str] = {}
    in_properties = False
    block: list[str] | None = None
    seen: set[int] = set()
    selected: dict[int, tuple[int, bytes]] = {}
    character_count = 0
    with path.open("rt", encoding="utf-8", newline="") as stream:
        for line_number, raw_line in enumerate(stream, 1):
            line = raw_line.rstrip("\r\n")
            if block is not None:
                block.append(line)
                if line == "ENDCHAR":
                    try:
                        encoding, advance, bounding_box, bitmap_rows = _parse_bdf_glyph(block)
                    except ValueError as exc:
                        raise ValueError(f"{path}:{line_number}: {exc}") from exc
                    character_count += 1
                    if encoding >= 0:
                        if encoding in seen:
                            raise ValueError(f"duplicate BDF encoding U+{encoding:04X}")
                        seen.add(encoding)
                        if encoding in required:
                            selected[encoding] = convert_bdf_glyph(advance, bounding_box, bitmap_rows)
                    block = None
                continue
            if line.startswith("STARTCHAR "):
                block = [line]
            elif line.startswith("FONT "):
                font_name = line.removeprefix("FONT ")
            elif line.startswith("SIZE "):
                fields = line.split()
                if len(fields) != 4:
                    raise ValueError("invalid BDF SIZE record")
                size = tuple(int(value) for value in fields[1:])
            elif line.startswith("FONTBOUNDINGBOX "):
                fields = line.split()
                if len(fields) != 5:
                    raise ValueError("invalid BDF FONTBOUNDINGBOX record")
                font_box = tuple(int(value) for value in fields[1:])
            elif line.startswith("CHARS "):
                expected_characters = int(line.split()[1])
            elif line.startswith("STARTPROPERTIES "):
                in_properties = True
            elif line == "ENDPROPERTIES":
                in_properties = False
            elif in_properties and " " in line:
                key, value = line.split(" ", 1)
                properties[key] = value.strip('"')
    if block is not None:
        raise ValueError(f"unterminated BDF glyph in {path}")
    expected_metadata = {
        "FONT_VERSION": "2026.07.20",
        "FAMILY_NAME": "Fusion Pixel 12px Mono zh_hans",
        "FONT_ASCENT": "10",
        "FONT_DESCENT": "2",
        "DEFAULT_CHAR": "-1",
    }
    if font_name != FUSION_XLFD or size != (12, 75, 75) or font_box != (12, 12, 0, -2):
        raise ValueError("unexpected Fusion Pixel Font BDF global metrics")
    if any(properties.get(key) != value for key, value in expected_metadata.items()):
        raise ValueError("unexpected Fusion Pixel Font BDF properties")
    if expected_characters != 36500 or character_count != expected_characters:
        raise ValueError(
            f"Fusion Pixel Font BDF glyph count mismatch: "
            f"header={expected_characters} parsed={character_count}"
        )
    return selected


def convert_unifont_glyph(width: int, rows: tuple[int, ...]) -> tuple[int, bytes]:
    # A 16x16 1-bit bitmap is resampled to TPT's 12-pixel font height. The
    # width follows the same 3:4 ratio so CJK glyphs stay square. Area
    # coverage is used deliberately: integer point sampling skipped source
    # rows 3, 7, 11 and 15 and could erase a one-pixel CJK stroke entirely.
    output_width = max(1, round(width * FONT_HEIGHT / 16))
    pixels: list[int] = []
    for output_y in range(FONT_HEIGHT):
        for output_x in range(output_width):
            # X coordinates use output_width units; Y coordinates use
            # FONT_HEIGHT units. Multiplying the overlap lengths yields an
            # exact integer coverage numerator over one output pixel.
            target_x0, target_x1 = output_x * width, (output_x + 1) * width
            target_y0, target_y1 = output_y * 16, (output_y + 1) * 16
            coverage = 0
            for source_y in range(16):
                overlap_y = max(0, min(target_y1, (source_y + 1) * FONT_HEIGHT) - max(target_y0, source_y * FONT_HEIGHT))
                if not overlap_y:
                    continue
                row = rows[source_y]
                for source_x in range(width):
                    if not row & (1 << (width - 1 - source_x)):
                        continue
                    overlap_x = max(0, min(target_x1, (source_x + 1) * output_width) - max(target_x0, source_x * output_width))
                    coverage += overlap_x * overlap_y
            # Round normalized coverage to the engine's 0..3 alpha range.
            # ``coverage`` is measured in a common source/target coordinate
            # system. One target pixel spans ``width * 16`` units.
            denominator = width * 16
            pixels.append((coverage * 3 + denominator // 2) // denominator)
    return output_width, pack_tpt_pixels(pixels)


def pack_tpt_pixels(pixels: Sequence[int]) -> bytes:
    """Pack pixels in FontReader::NextPixel order (first pixel in bits 0..1)."""
    packed = bytearray()
    for index in range(0, len(pixels), 4):
        value = 0
        for offset, pixel in enumerate(pixels[index : index + 4]):
            value |= (pixel & 0x3) << (offset * 2)
        packed.append(value)
    return bytes(packed)


def unpack_tpt_glyph(width: int, bitmap: bytes) -> tuple[tuple[int, ...], ...]:
    """Decode a TPT glyph exactly as FontReader::NextPixel does."""
    if width < 1 or len(bitmap) != width * 3:
        raise ValueError("invalid TPT glyph bitmap")
    pixels = [
        (bitmap[index // 4] >> ((index % 4) * 2)) & 0x3
        for index in range(width * FONT_HEIGHT)
    ]
    return tuple(
        tuple(pixels[row * width : (row + 1) * width])
        for row in range(FONT_HEIGHT)
    )


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
    verify_pinned_inputs(source_root)
    upstream_path = source_root / UPSTREAM_FONT
    unifont_path = source_root / UNIFONT_HEX
    fusion_path = source_root / FUSION_BDF
    language_paths = sorted((source_root / "src/lang").glob("*.json"))
    if not language_paths:
        raise ValueError("no embedded language catalogs found")
    glyphs = parse_tpt_font(upstream_path)
    required = required_codepoints(language_paths)
    periodic_required = periodic_codepoints(source_root / "docs/PERIODIC_ELEMENT_SOURCE_MAP.csv")
    required.update(periodic_required)
    player_text_required, player_text_zh_required = player_text_codepoints(source_root)
    required.update(player_text_required)
    zh_path = source_root / "src/lang/zh-CN.json"
    if not zh_path.is_file():
        raise ValueError("Simplified Chinese language catalog is missing")
    zh_required = required_codepoints([zh_path])
    zh_required.update(periodic_required)
    zh_required.update(player_text_zh_required)
    missing = required - set(glyphs)
    fusion = parse_fusion_bdf(fusion_path, missing)
    missing_zh_fusion = sorted((zh_required - set(glyphs)) - set(fusion))
    if missing_zh_fusion:
        rendered = ", ".join(f"U+{codepoint:04X}" for codepoint in missing_zh_fusion[:12])
        suffix = "" if len(missing_zh_fusion) <= 12 else f" (+{len(missing_zh_fusion) - 12})"
        raise ValueError(f"Fusion Pixel Font does not cover Simplified Chinese catalog: {rendered}{suffix}")
    unifont_needed = missing - set(fusion)
    unifont = parse_unifont(unifont_path, unifont_needed)
    unresolved = sorted(unifont_needed - set(unifont))
    if unresolved:
        rendered = ", ".join(f"U+{codepoint:04X}" for codepoint in unresolved[:12])
        suffix = "" if len(unresolved) <= 12 else f" (+{len(unresolved) - 12})"
        raise ValueError(f"GNU Unifont does not cover embedded language glyphs: {rendered}{suffix}")
    for codepoint in missing:
        glyphs[codepoint] = fusion.get(codepoint) or convert_unifont_glyph(*unifont[codepoint])

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(encode_tpt_font(glyphs))
    return {
        "base_glyphs": len(glyphs) - len(missing),
        "added_fusion_glyphs": len(fusion),
        "added_unifont_glyphs": len(unifont),
        "glyphs": len(glyphs),
        "required_glyphs": len(required),
        "periodic_required_glyphs": len(periodic_required),
        "player_text_required_glyphs": len(player_text_required),
        "player_text_zh_required_glyphs": len(player_text_zh_required),
        "output_bytes": output.stat().st_size,
        "output_sha256": sha256(output),
        "upstream_font_sha256": sha256(upstream_path),
        "fusion_bdf_sha256": sha256(fusion_path),
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
