#!/usr/bin/env python3
"""Fail-closed checks for OmniPack's bounded advanced nuclear module."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_ELEMENTS = {
    328: "NFUL",
    329: "MODR",
    330: "CROD",
    331: "NCLT",
    332: "NWST",
    333: "NGEN",
    334: "RSHD",
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
    except (OSError, csv.Error) as exc:
        errors.append(f"{path}: cannot read registry: {exc}")
        return
    by_id: dict[int, dict[str, str]] = {}
    for row in rows:
        try:
            by_id[int(row.get("stable_id", ""))] = row
        except ValueError:
            continue
    for stable_id, name in EXPECTED_ELEMENTS.items():
        row = by_id.get(stable_id)
        if row is None:
            errors.append(f"{path}: missing advanced-nuclear stable ID {stable_id}")
            continue
        expected = {
            "identifier": f"OMNI_PT_{name}",
            "meson_name": name,
            "source_file": f"src/simulation/elements/{name}.cpp",
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
    engine = root / "src" / "simulation" / "OmniNuclear.cpp"
    text = read_text(engine, errors)
    if not text:
        return
    required_markers = {
        "per-frame event budget": "NuclearEventsPerFrame = 512",
        "bounded local lookup": "for (int ry = -1; ry <= 1; ++ry)",
        "deterministic cascade guard": "tmp3 == sim->currentTick + 1",
        "moderated fuel": "PT_NFUL",
        "moderator condition": "PT_MODR",
        "control-rod condition": "PT_CROD",
        "coolant route": "PT_NCLT",
        "waste route": "PT_NWST",
        "generator route": "PT_NGEN",
        "shield route": "PT_RSHD",
        "official neutron": "PT_NEUT",
        "official steam": "PT_WTRV",
        "fuel requirement for source": "if (fuel.index < 0)",
        "neutron output occupancy guard": "!photons[y + ry][x + rx]",
        "one neutron per spark guard": "parts[i].tmp4 & NuclearSparkEmitted",
    }
    for label, marker in required_markers.items():
        if marker not in text:
            errors.append(f"{engine}: missing {label}: {marker!r}")
    forbidden = {
        "global particle scan": re.compile(r"\bNPART\b|\bparts\.active\b"),
        "large neighbourhood": re.compile(r"(?:rx|ry)\s*[<=>!]+\s*[2-9]"),
        "official nuclear hook": re.compile(r"OmniNuclear.*(?:URAN|PLUT|DEUT)"),
    }
    for label, pattern in forbidden.items():
        if pattern.search(text):
            errors.append(f"{engine}: forbidden {label} detected")

    spark = read_text(root / "src" / "simulation" / "elements" / "SPRK.cpp", errors)
    if "ct == PT_NGEN" not in spark or "OmniNuclearSparkUpdate" not in spark:
        errors.append("SPRK.cpp: sparked neutron-generator hook is missing")
    for name in EXPECTED_ELEMENTS.values():
        path = root / "src" / "simulation" / "elements" / f"{name}.cpp"
        element = read_text(path, errors)
        if "HeatCapacity =" not in element:
            errors.append(f"{path}: explicit HeatCapacity is missing")
        if f'Identifier = "OMNI_PT_{name}"' not in element:
            errors.append(f"{path}: expected stable identifier is missing")


def audit(root: Path) -> list[str]:
    errors: list[str] = []
    check_registry(root, errors)
    check_engine(root, errors)
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit bounded advanced-nuclear IDs, routes and performance guards."
    )
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
            print(f"nuclear-audit: ERROR {error}", file=sys.stderr)
        print(f"nuclear-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("nuclear-audit: PASS (7 elements, 4 bounded reactor paths, 3x3 local)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
