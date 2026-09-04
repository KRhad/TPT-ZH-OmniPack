#!/usr/bin/env python3
"""Reject encoding, PDF-extraction and font-coverage defects in element notes."""

from __future__ import annotations

import bz2
import json
import re
import unicodedata
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DESCRIPTION_FILE = ROOT / "resources" / "element_descriptions_zh_CN.json"
FONT_FILE = ROOT / "src" / "graphics" / "font_bz2.cpp"

BAD_PATTERNS = {
    "replacement or mojibake": r"�|Ã|Â|â€|锟斤拷|烫烫烫|屯屯屯",
    "known PDF OCR token": (
        r"\b(?:ENUT|SADN|WATA|WARE|BRAYA|BRTM|GAUS|IMWR|PTOH|QRYN|"
        r"RPTI|RQRT|Llfe|CLFM|ATMR|HYGH|UNG)\b"
    ),
    "unexpanded placeholder": r"AAAA|Molten[A-Za-z]+",
    "split technical word": r"\b(?:T mp|Tem p|C type|Li fe|L ife|Cype)\b",
    "raw table fragment": r"\b(?:no eff|scat|Pressure|Snapshot\d*|Stamps/Saves)\b",
    "collapsed FILT table": r"颜色\d\(|改变\d\(|光谱中\d\(",
    "joined sentence": r"石油气使|木材使|SAWD使|反应当|STNE 此|时 SALT，当",
    "nonstandard temperature unit": r"(?<=\d)k\b|(?<=\d)C\b",
    "damaged hexadecimal prefix": r"\b0×[0-9A-Fa-f]",
}


def font_codepoints() -> set[int]:
    source = FONT_FILE.read_text(encoding="utf-8")
    values = bytes(map(int, re.findall(r"(?<![A-Za-z_])\d+(?![A-Za-z_])", source.split("{{{", 1)[1])))
    decoded = bz2.decompress(values)
    result: set[int] = set()
    offset = 0
    while offset < len(decoded):
        codepoint = decoded[offset] | decoded[offset + 1] << 8 | decoded[offset + 2] << 16
        width = decoded[offset + 3]
        result.add(codepoint)
        offset += 4 + width * 3
    return result


def main() -> None:
    payload = json.loads(DESCRIPTION_FILE.read_text(encoding="utf-8"))
    descriptions: dict[str, str] = payload["descriptions"]
    failures: list[str] = []
    if len(descriptions) != 205:
        failures.append(f"expected 205 descriptions, found {len(descriptions)}")

    for label, pattern in BAD_PATTERNS.items():
        matches = sorted(code for code, text in descriptions.items() if re.search(pattern, text))
        if matches:
            failures.append(f"{label}: {' '.join(matches)}")

    for code, text in descriptions.items():
        for char in text:
            if unicodedata.category(char) in {"Cc", "Cf", "Cs", "Co", "Cn"} and char not in "\n\t":
                failures.append(f"abnormal U+{ord(char):04X} in {code}")

    available = font_codepoints()
    used = {ord(char) for text in descriptions.values() for char in text if ord(char) > 127}
    missing = sorted(used - available)
    if missing:
        failures.append("missing glyphs: " + " ".join(f"U+{value:04X}" for value in missing))

    if failures:
        raise SystemExit("description audit failed:\n- " + "\n- ".join(failures))
    print(
        f"description audit passed: elements={len(descriptions)} "
        f"non_ascii={len(used)} missing_glyphs=0"
    )


if __name__ == "__main__":
    main()
