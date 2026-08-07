#!/usr/bin/env python3
"""Validate explicit periodic-table links and the material-picker integration."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Sequence


FIELDS = (
    "tool_identifier", "content_kind", "primary_atomic_number",
    "related_atomic_numbers", "compound_group", "display_order",
    "periodic_visible", "formula", "zh_name", "en_name",
)
KINDS = {"periodic_element", "isotope", "inorganic_compound"}
GROUPS = {
    "element", "isotope", "oxide", "hydroxide", "acid", "base", "salt",
    "halide", "sulfide", "nitride", "carbide", "hydride", "other",
}


def rows(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != FIELDS:
                errors.append(f"{path}: unexpected columns {reader.fieldnames}")
                return []
            result = list(reader)
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []
    for line_number, row in enumerate(result, 2):
        if None in row or any(row[field] is None for field in FIELDS):
            errors.append(f"{path}:{line_number}: row width does not match header")
            continue
        row.update({field: row[field].strip() for field in FIELDS})
    return result


def generic_rows(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            return list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []


def audit(root: Path) -> tuple[list[str], dict[str, int]]:
    errors: list[str] = []
    link_rows = rows(root / "docs" / "PERIODIC_CONTENT_LINKS.csv", errors)
    registry = {
        row["identifier"]: row
        for row in generic_rows(root / "docs" / "ELEMENT_REGISTRY.csv", errors)
    }
    policy = {
        row["identifier"]: row
        for row in generic_rows(root / "docs" / "CONTENT_MENU_POLICY.csv", errors)
    }
    periodic_map = generic_rows(root / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv", errors)
    isotope_rows = generic_rows(root / "docs" / "ISOTOPE_REGISTRY.csv", errors)
    if errors:
        return errors, {}

    by_identifier: dict[str, dict[str, str]] = {}
    related_counts = {atomic_number: 0 for atomic_number in range(1, 119)}
    for line_number, row in enumerate(link_rows, 2):
        identifier = row["tool_identifier"]
        if not identifier or identifier in by_identifier:
            errors.append(f"periodic links:{line_number}: empty or duplicate identifier {identifier}")
            continue
        by_identifier[identifier] = row
        if row["content_kind"] not in KINDS:
            errors.append(f"periodic links:{line_number}: invalid content kind")
        if row["compound_group"] not in GROUPS:
            errors.append(f"periodic links:{line_number}: invalid group")
        try:
            atoms = [int(value) for value in row["related_atomic_numbers"].split("|")]
            primary = int(row["primary_atomic_number"])
            order = int(row["display_order"])
        except ValueError:
            errors.append(f"periodic links:{line_number}: invalid numeric value")
            continue
        if atoms != sorted(set(atoms)) or not atoms or not all(1 <= atom <= 118 for atom in atoms):
            errors.append(f"periodic links:{line_number}: invalid related atoms")
        if primary not in atoms or order < 0:
            errors.append(f"periodic links:{line_number}: invalid primary atom or order")
        if row["periodic_visible"] not in {"true", "false"}:
            errors.append(f"periodic links:{line_number}: invalid periodic_visible")
        if not row["formula"] or not row["zh_name"] or not row["en_name"]:
            errors.append(f"periodic links:{line_number}: empty display metadata")
        element = registry.get(identifier)
        if not element or element.get("implementation_status") != "implemented":
            errors.append(f"periodic links:{line_number}: unknown or unimplemented tool {identifier}")
        for atom in atoms:
            related_counts[atom] += 1

    expected_elements: dict[str, str] = {}
    for row in periodic_map:
        identifier = row["official_mapping"] or row["omnipack_mapping"]
        expected_elements[identifier] = row["atomic_number"]
        link = by_identifier.get(identifier)
        if not link or link["content_kind"] != "periodic_element":
            errors.append(f"periodic element picker missing: {identifier}")
        elif link["related_atomic_numbers"] != row["atomic_number"]:
            errors.append(f"periodic element relation mismatch: {identifier}")
    actual_elements = {
        identifier for identifier, row in by_identifier.items()
        if row["content_kind"] == "periodic_element"
    }
    if actual_elements != set(expected_elements):
        errors.append("periodic element link set does not match the 118-row source map")

    expected_isotopes = {
        row["identifier"]: row["atomic_number"]
        for row in isotope_rows if row.get("status") == "implemented"
    }
    actual_isotopes = {
        identifier: row["related_atomic_numbers"]
        for identifier, row in by_identifier.items()
        if row["content_kind"] == "isotope"
    }
    if actual_isotopes != expected_isotopes:
        errors.append("isotope link set does not match ISOTOPE_REGISTRY.csv")

    policy_inorganic = {
        identifier for identifier, row in policy.items()
        if row.get("content_kind") == "inorganic_compound"
    }
    official_inorganic = {"DEFAULT_PT_WATR", "DEFAULT_PT_CO2", "DEFAULT_PT_SALT"}
    actual_inorganic = {
        identifier for identifier, row in by_identifier.items()
        if row["content_kind"] == "inorganic_compound"
    }
    if actual_inorganic != policy_inorganic | official_inorganic:
        missing = sorted((policy_inorganic | official_inorganic) - actual_inorganic)
        extra = sorted(actual_inorganic - (policy_inorganic | official_inorganic))
        errors.append(f"inorganic link set mismatch: missing={missing} extra={extra}")

    forbidden = {
        identifier for identifier, row in policy.items()
        if row.get("content_kind") in {"organic_material", "alloy_engineering", "custom_special"}
    }
    wrong = sorted(forbidden & set(by_identifier))
    if wrong:
        errors.append(f"periodic table includes excluded content: {wrong}")
    if any(count == 0 for count in related_counts.values()):
        errors.append("one or more periodic elements has no selectable content")
    groups_by_atom = {atomic_number: set() for atomic_number in range(1, 119)}
    for row in link_rows:
        for value in row["related_atomic_numbers"].split("|"):
            groups_by_atom[int(value)].add(row["compound_group"])
    required_smoke_groups = {
        1: {"element", "isotope", "acid"},
        6: {"element", "isotope", "oxide", "acid"},
        8: {"element", "oxide", "hydroxide"},
        11: {"element", "hydroxide", "salt", "hydride"},
        17: {"element", "acid", "halide"},
        26: {"element", "oxide", "halide", "sulfide"},
        29: {"element", "oxide", "salt", "halide"},
        92: {"element", "oxide"},
    }
    for atomic_number, expected_groups in required_smoke_groups.items():
        missing_groups = expected_groups - groups_by_atom[atomic_number]
        if missing_groups:
            errors.append(
                f"periodic detail smoke page {atomic_number} misses groups "
                f"{sorted(missing_groups)}"
            )

    periodic_activity = (
        root / "src" / "gui" / "periodictable" / "PeriodicTableActivity.cpp"
    ).read_text(encoding="utf-8")
    detail_activity_path = (
        root / "src" / "gui" / "periodictable" / "PeriodicElementDetailActivity.cpp"
    )
    if not detail_activity_path.is_file():
        errors.append("periodic element detail activity is missing")
        detail_activity = ""
    else:
        detail_activity = detail_activity_path.read_text(encoding="utf-8")
    if "PeriodicElementDetailActivity" not in periodic_activity:
        errors.append("periodic table still lacks the material picker transition")
    if "gameController->SetActiveTool(0, tool)" in periodic_activity:
        errors.append("periodic grid still selects the elemental tool directly")
    if "SetLongPressCallback" in periodic_activity:
        errors.append("periodic grid still handles long press")
    for token in (
        "GetPeriodicContentLinks", "GetOmniElementSelectionRestriction",
        "ConfirmPrompt", "OpenOptions", "ui::ScrollPanel",
        'Tr("encyclopedia.description")', "elementTool->Description",
        "ElementDescriptionWithLongPressHint",
    ):
        if token not in detail_activity:
            errors.append(f"periodic detail integration is missing {token}")

    try:
        en = json.loads((root / "src" / "lang" / "en-US.json").read_text(encoding="utf-8"))
        zh = json.loads((root / "src" / "lang" / "zh-CN.json").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        errors.append(f"cannot load periodic localization: {exc}")
        en = {}
        zh = {}
    if en.get("periodic.table.search_placeholder") != "Search elements, symbols, or atomic numbers":
        errors.append("English periodic search placeholder is not player-facing")
    if zh.get("periodic.table.search_placeholder") != "搜索元素、符号或原子序数":
        errors.append("Chinese periodic search placeholder is not player-facing")
    for catalog in (en, zh):
        for key, value in catalog.items():
            if key.startswith("periodic.") and "identifier" in str(value).casefold():
                errors.append(f"developer term exposed in periodic UI: {key}")

    stats = {
        "links": len(link_rows),
        "periodic_elements": len(actual_elements),
        "isotopes": len(actual_isotopes),
        "inorganic_compounds": len(actual_inorganic),
    }
    return errors, stats


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors, stats = audit(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        print(f"periodic-content-links: FAIL errors={len(errors)}")
        return 1
    if not args.quiet:
        print("periodic-content-links: PASS " + " ".join(f"{key}={value}" for key, value in stats.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
