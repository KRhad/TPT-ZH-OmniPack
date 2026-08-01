#!/usr/bin/env python3
"""Fail-closed audit for mineral, ceramic, glass, and building materials."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_ELEMENTS = {
    521: "GYPS",
    522: "BAUX",
    523: "CUOR",
    524: "ZNOR",
    525: "PBOR",
    526: "UORE",
    527: "FELD",
    528: "CEMT",
    529: "ALCR",
    530: "BSGL",
    531: "QGLS",
    532: "RFBK",
}


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def check_registry(root: Path, errors: list[str]) -> None:
    path = root / "docs" / "ELEMENT_REGISTRY.csv"
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read registry: {exc}")
        return
    by_id = {int(row["stable_id"]): row for row in rows if row["stable_id"].isdigit()}
    for stable_id, name in EXPECTED_ELEMENTS.items():
        row = by_id.get(stable_id)
        if not row:
            errors.append(f"{path}: missing material stable ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{name}",
            "meson_name": name,
            "source_file": f"src/simulation/elements/{name}.cpp",
            "module": "metallurgy",
            "implementation_status": "implemented",
            "default_enabled": "true",
            "is_duplicate": "false",
        }
        for field, value in expected.items():
            if row.get(field) != value:
                errors.append(
                    f"{path}: ID {stable_id} {field}={row.get(field)!r}; "
                    f"expected {value!r}"
                )


def check_engine(root: Path, errors: list[str]) -> None:
    path = root / "src" / "simulation" / "OmniMaterials.cpp"
    text = read_text(path, errors)
    required = {
        "module gate": '"Omni.Modules.Metallurgy"',
        "3x3 local scan": "for (int ry = -1; ry <= 1; ++ry)",
        "event budget": "MaterialEventsPerFrame = 1536",
        "gypsum cycle": "UpdateGypsum(",
        "bounded ore helper": "ReduceOre(",
        "feldspar glassmaking": "UpdateFeldspar(",
        "cement curing": "UpdateCement(",
        "special glass pressure": "UpdateSpecialGlass(",
        "refractory quench": "UpdateRefractoryBrick(",
        "molten quartz quench": "parts[i].ctype != PT_QRTZ",
        "alumina sintering": "PT_ALCR",
        "borosilicate casting": "PT_BSGL",
        "refractory firing": "PT_RFBK",
    }
    for label, marker in required.items():
        if marker not in text:
            errors.append(f"{path}: missing {label}: {marker!r}")
    if re.search(r"\bNPART\b|parts\.active|for\s*\([^\n]*PT_NUM", text):
        errors.append(f"{path}: global particle or element scan is forbidden")
    if "sim->create_part" not in text or "ConsumeEventBudget(sim)" not in text:
        errors.append(f"{path}: particle creation is not visibly budgeted")

    integrations = {
        root / "src" / "simulation" / "elements" / "FIRE.cpp":
            "OmniMaterialsLavaUpdate",
        root / "src" / "simulation" / "elements" / "SPRK.cpp":
            "OmniMaterialsSparkUpdate",
        root / "src" / "simulation" / "Simulation.cpp":
            "IsRefractingGlass",
        root / "src" / "simulation" / "OmniChemistry.cpp":
            "PT_BSGL, PT_QGLS",
    }
    for integration_path, marker in integrations.items():
        if marker not in read_text(integration_path, errors):
            errors.append(f"{integration_path}: missing material integration {marker!r}")

    for name in EXPECTED_ELEMENTS.values():
        element_path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        element = read_text(element_path, errors)
        for marker in (
            f'Identifier = "OMNI_PT_{name}"',
            "HeatCapacity =",
            "Description = Localization::Ref().Tr",
            "Update = &OmniMaterialsElementUpdate",
        ):
            if marker not in element:
                errors.append(f"{element_path}: missing {marker!r}")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registry(root, errors)
    check_engine(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"materials-audit: ERROR {error}", file=sys.stderr)
        print(f"materials-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("materials-audit: PASS (12 stable elements, 3x3 local, 1536/frame)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
