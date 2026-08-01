#!/usr/bin/env python3
"""Fail-closed audit for the 10-bit element capacity and high-ID save contract."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def check_constants(root: Path, errors: list[str]) -> None:
    defs = read_text(root / "src/simulation/ElementDefs.h", errors)
    content = read_text(root / "src/gui/game/OmniContent.h", errors)
    content_cpp = read_text(root / "src/gui/game/OmniContent.cpp", errors)
    markers = {
        "10-bit pmap": "constexpr int PMAPBITS = 10;",
        "derived element count": "constexpr int PT_NUM = 1 << PMAPBITS;",
        "signed pmap safety": "NPART - 1 <= (INT32_MAX >> PMAPBITS)",
    }
    for label, marker in markers.items():
        if marker not in defs:
            errors.append(f"ElementDefs.h: missing {label}: {marker!r}")
    range_markers = {
        "engineering first": "OmniEngineeringFirstId = 512",
        "engineering last": "OmniEngineeringLastId = 575",
        "isotope first": "OmniIsotopeFirstId = 576",
        "isotope last": "OmniIsotopeLastId = 588",
        "future first": "OmniFutureContentFirstId = 589",
        "future last": "OmniFutureContentLastId = 1023",
    }
    for label, marker in range_markers.items():
        if marker not in content:
            errors.append(f"OmniContent.h: missing {label}: {marker!r}")
    if "return OmniElementModule::Metallurgy;" not in content_cpp or (
        "elementId <= OmniEngineeringLastId" not in content_cpp
    ):
        errors.append("OmniContent.cpp: engineering IDs are not routed to metallurgy")
    if "elementId <= OmniIsotopeLastId" not in content_cpp:
        errors.append("OmniContent.cpp: isotope IDs are not routed to advanced nuclear")


def check_save_path(root: Path, errors: list[str]) -> None:
    save = read_text(root / "src/client/GameSave.cpp", errors)
    markers = {
        "minimum width": "MinimumOpsPmapBits = 8",
        "maximum width": "MaximumOpsPmapBits = 16",
        "fail-closed lower bound": "pmapbits < MinimumOpsPmapBits",
        "fail-closed upper bound": "pmapbits > MaximumOpsPmapBits",
        "unsigned source unpack": "auto packed = static_cast<uint32_t>(*prop);",
        "source-width extraction": "auto extra = packed >> pmapbits;",
        "current-width repack": "extra << PMAPBITS",
        "source-width palette lookup": (
            "std::vector<int> partMap(UINT32_C(1) << pmapbits, 0);"
        ),
        "source-width palette entries": (
            "static_cast<std::size_t>(pi.second) < partMap.size()"
        ),
        "direct type maps before range filtering": "tempPart.type = paletteLookup(tempPart.type, false);",
        "mapped direct type range guard": "if (tempPart.type <= 0 || tempPart.type >= PT_NUM)",
        "second direct type byte read": "particles[newIndex].type |= (((unsigned)partsData[i++]) << 8)",
        "second direct type byte write": "if (part.type & 0xFF00)",
        "identifier palette": "elements[ID].Identifier",
    }
    for label, marker in markers.items():
        if marker not in save:
            errors.append(f"GameSave.cpp: missing {label}: {marker!r}")


def check_registration(root: Path, errors: list[str]) -> None:
    meson = read_text(root / "src/simulation/elements/meson.build", errors)
    entries = []
    match = re.search(
        r"simulation_elem_names\s*=\s*\[(.*?)^\]", meson, re.MULTILINE | re.DOTALL
    )
    if not match:
        errors.append("elements/meson.build: element list is missing")
    else:
        for raw in match.group(1).splitlines():
            entry = raw.split("#", 1)[0].strip().rstrip(",").strip()
            if entry:
                entries.append(entry)
        if len(entries) <= 512 or entries[512] != "'SOLD'":
            errors.append("elements/meson.build: stable slot 512 must be SOLD")

    registry_path = root / "docs/ELEMENT_REGISTRY.csv"
    try:
        with registry_path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, csv.Error) as exc:
        errors.append(f"{registry_path}: cannot read registry: {exc}")
        rows = []
    high = [row for row in rows if row.get("stable_id") == "512"]
    if len(high) != 1 or high[0].get("identifier") != "OMNI_PT_SOLD":
        errors.append("ELEMENT_REGISTRY.csv: stable slot 512 must uniquely map to SOLD")

    generator = read_text(root / "tools/generate_element_catalog.py", errors)
    if "ELEMENT_LIMIT = 1024" not in generator:
        errors.append("generate_element_catalog.py: catalog limit is not 1024")


def check_carriers_and_lua(root: Path, errors: list[str]) -> None:
    carriers = {
        "LAVA.cpp": "FIELD_CTYPE",
        "SPRK.cpp": "FIELD_CTYPE",
        "BRMT.cpp": "FIELD_CTYPE",
        "CONV.cpp": "FIELD_TMP",
        "VIRS.cpp": "FIELD_TMP2",
    }
    for filename, marker in carriers.items():
        text = read_text(root / "src/simulation/elements" / filename, errors)
        if marker not in text:
            errors.append(f"{filename}: high-ID carrier declaration lacks {marker}")

    lua = read_text(root / "src/lua/LuaElements.cpp", errors)
    for marker in (
        "LuaPreferredElementFirstId = 196",
        "LuaPreferredElementLastId = 255",
        "i >= LuaPreferredElementFirstId",
        "i > LuaPreferredElementLastId",
        "Never consume disabled official IDs",
        "if (id < 0 || id >= PT_NUM)",
    ):
        if marker not in lua:
            errors.append(f"LuaElements.cpp: missing capacity marker {marker!r}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_constants(root, errors)
    check_save_path(root, errors)
    check_registration(root, errors)
    check_carriers_and_lua(root, errors)
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"element-capacity-audit: ERROR {error}", file=sys.stderr)
        print(f"element-capacity-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print(
            "element-capacity-audit: PASS "
            "pmapbits=10 pt_num=1024 high_id=512 "
            "can_move_bytes=1048576 lua_can_move_bytes=1048576"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
