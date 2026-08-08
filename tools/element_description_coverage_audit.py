#!/usr/bin/env python3
"""Audit full-detail coverage without replacing concise main-menu tooltips."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Sequence


OFFICIAL_SOURCE = "The-Powder-Toy/The-Powder-Toy"
REQUIRED_CONTENT_FIELDS = (
    "recipe_en", "recipe_zh", "use_en", "use_zh", "hazard_en", "hazard_zh",
)
FULL_DESCRIPTION_FIELDS = (
    "identifier", "wiki_url", "wiki_snapshot", "english_description",
    "chinese_description",
)


def load_csv(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            return list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []


def audit(root: Path) -> tuple[list[str], dict[str, int]]:
    errors: list[str] = []
    registry_rows = load_csv(root / "docs" / "ELEMENT_REGISTRY.csv", errors)
    content_rows = load_csv(root / "docs" / "ELEMENT_CONTENT.csv", errors)
    full_rows = load_csv(
        root / "docs" / "OFFICIAL_ELEMENT_DESCRIPTIONS.csv", errors
    )
    if errors:
        return errors, {}

    content = {row.get("identifier", ""): row for row in content_rows}
    full = {row.get("identifier", ""): row for row in full_rows}
    if any(tuple(row.keys()) != FULL_DESCRIPTION_FIELDS for row in full_rows):
        errors.append("official full-description columns are not canonical")

    canonical = [
        row for row in registry_rows
        if row.get("implementation_status") == "implemented"
        and row.get("is_duplicate") != "true"
    ]
    official = [row for row in canonical if row.get("source_mod") == OFFICIAL_SOURCE]
    omni = [row for row in canonical if row.get("source_mod") != OFFICIAL_SOURCE]

    for row in canonical:
        identifier = row.get("identifier", "")
        for field in ("english_description", "chinese_description"):
            if not (row.get(field) or "").strip():
                errors.append(f"{identifier}: empty bilingual summary field {field}")

    expected_full = {row["identifier"] for row in official}
    if set(full) != expected_full:
        errors.append(
            "official full-description set does not match canonical implemented elements"
        )
    for identifier, row in full.items():
        for field in FULL_DESCRIPTION_FIELDS:
            if not (row.get(field) or "").strip():
                errors.append(f"{identifier}: empty full-description field {field}")

    for row in omni:
        identifier = row["identifier"]
        record = content.get(identifier)
        if not record:
            errors.append(f"{identifier}: missing structured full-detail content")
            continue
        for field in REQUIRED_CONTENT_FIELDS:
            if not (record.get(field) or "").strip():
                errors.append(f"{identifier}: empty structured content field {field}")
        production = tuple(
            bool((record.get(field) or "").strip())
            for field in ("production_en", "production_zh")
        )
        if production[0] != production[1]:
            errors.append(f"{identifier}: production content is not bilingual")

    element_info_path = root / "src" / "gui" / "elementsearch" / "ElementInfo.cpp"
    try:
        element_info = element_info_path.read_text(encoding="utf-8")
    except OSError as exc:
        errors.append(f"{element_info_path}: cannot read: {exc}")
        element_info = ""
    for marker in (
        'Tr("encyclopedia.simulation_properties")',
        'Tr("encyclopedia.spawn_temperature")',
        'Tr("encyclopedia.weight")',
        'Tr("encyclopedia.gravity")',
        'Tr("encyclopedia.hardness")',
        'Tr("encyclopedia.flammability")',
        'Tr("encyclopedia.explosiveness")',
        'Tr("encyclopedia.conductive")',
        'Tr("encyclopedia.neutron_absorbing")',
        'Tr("encyclopedia.low_pressure")',
        'Tr("encyclopedia.high_pressure")',
        "TransitionTarget(element.LowTemperatureTransition)",
        "TransitionTarget(element.HighTemperatureTransition)",
    ):
        if marker not in element_info:
            errors.append(f"full-detail runtime property coverage misses {marker}")

    periodic_detail_path = (
        root / "src" / "gui" / "periodictable"
        / "PeriodicElementDetailActivity.cpp"
    )
    try:
        periodic_detail = periodic_detail_path.read_text(encoding="utf-8")
    except OSError as exc:
        errors.append(f"{periodic_detail_path}: cannot read: {exc}")
        periodic_detail = ""
    if 'String::Build("· ", Localization::Ref().Tr("element.long_press_hint"))' not in periodic_detail:
        errors.append("periodic picker lacks the standalone long-press hint")
    for forbidden in ('Tr("encyclopedia.description")', "elementTool->Description"):
        if forbidden in periodic_detail:
            errors.append(f"periodic picker repeats description via {forbidden}")

    stats = {
        "canonical_materials": len(canonical),
        "official_wiki_full": len(full),
        "official_runtime_detail": len(official) - len(full),
        "omnipack_structured_detail": len(omni),
    }
    if sum(
        stats[key]
        for key in (
            "official_wiki_full", "official_runtime_detail",
            "omnipack_structured_detail",
        )
    ) != stats["canonical_materials"]:
        errors.append("full-detail coverage counts do not cover every canonical material")
    return errors, stats


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors, stats = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"element-description-coverage: ERROR {error}")
        print(f"element-description-coverage: FAIL ({len(errors)} errors)")
        return 1
    if not args.quiet:
        print(
            "element-description-coverage: PASS "
            + " ".join(f"{key}={value}" for key, value in stats.items())
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
