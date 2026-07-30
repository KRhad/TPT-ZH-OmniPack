#!/usr/bin/env python3
"""Fail-closed checks for OmniPack's bounded local biology module."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


EXPECTED_ELEMENTS = {
    288: "NUTR",
    289: "ALGA",
    290: "MYCL",
    291: "SPOR",
    292: "PATH",
    293: "STER",
    294: "HUMS",
    295: "BIOF",
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
            errors.append(f"{path}: missing biology stable ID {stable_id}")
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
    engine = root / "src" / "simulation" / "OmniBiology.cpp"
    text = read_text(engine, errors)
    if not text:
        return
    required_markers = {
        "per-frame event budget": "BiologyEventsPerFrame = 1024",
        "bounded local lookup": "for (int ry = -1; ry <= 1; ++ry)",
        "deterministic cascade guard": "tmp3 == sim->currentTick + 1",
        "simplified simulation preference": "Omni.Simulation.SimplifiedBiology",
        "algae photosynthesis": "PT_ALGA",
        "mycelium decomposition": "PT_MYCL",
        "humus fertilizer recovery": "HumusFertilizerRecovery",
        "chemistry fertilizer input": "PT_FERT",
        "spore germination": "PT_SPOR",
        "pathogen infection": "PT_PATH",
        "sterilant treatment": "PT_STER",
        "biofilm filtration": "PT_BIOF",
        "stable humus output": "PT_HUMS",
        "official water input": "PT_WATR",
        "official carbon dioxide input": "PT_CO2",
        "official oxygen output": "PT_O2",
    }
    for label, marker in required_markers.items():
        if marker not in text:
            errors.append(f"{engine}: missing {label}: {marker!r}")

    forbidden = {
        "global particle scan": re.compile(r"\bNPART\b|\bparts\.active\b"),
        "large neighbourhood": re.compile(r"(?:rx|ry)\s*[<=>!]+\s*[2-9]"),
        "official biology hook": re.compile(r"OmniBiology.*(?:PLNT|VIRS|LIFE)"),
    }
    for label, pattern in forbidden.items():
        if pattern.search(text):
            errors.append(f"{engine}: forbidden {label} detected")

    settings = read_text(root / "src" / "gui" / "game" / "OmniContent.cpp", errors)
    if not re.search(
        r"OmniSetting::SimplifiedBiology.*?false\s*,\s*true",
        settings,
    ):
        errors.append("OmniContent.cpp: simplified biology setting is not available")

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
        description="Audit bounded biology IDs, ecology paths and performance guards."
    )
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"biology-audit: ERROR {error}", file=sys.stderr)
        print(f"biology-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("biology-audit: PASS (8 elements, 7 bounded ecology/integration paths, 3x3 local)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
