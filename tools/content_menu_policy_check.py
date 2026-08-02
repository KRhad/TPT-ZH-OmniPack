#!/usr/bin/env python3
"""Validate centralized menu placement without changing element definitions."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import re
from typing import Sequence


POLICY_FIELDS = (
    "identifier", "stable_id", "content_kind", "main_menu_policy",
    "periodic_atomic_numbers", "standalone_menu",
)
BASELINE_FIELDS = ("identifier", "stable_id", "menu_section", "menu_visible")
EXPECTED_KIND_POLICY = {
    "periodic_element": {"periodic_only"},
    "isotope": {"periodic_only"},
    "inorganic_compound": {"periodic_only"},
    "organic_material": {"organic_menu"},
    "alloy_engineering": {"alloy_menu", "existing_dedicated_menu"},
    "ecology_material": {"preserve_existing"},
    "nuclear_device": {"existing_dedicated_menu"},
    "custom_special": {"preserve_existing"},
    "official": {"official_default"},
    "compatibility_alias": {"hidden"},
    "hidden_internal": {"hidden"},
}


def read_csv(path: Path, fields: tuple[str, ...], errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != fields:
                errors.append(f"{path}: expected columns {fields}, got {reader.fieldnames}")
                return []
            rows = list(reader)
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []
    for line_number, row in enumerate(rows, 2):
        if None in row or any(row[field] is None for field in fields):
            errors.append(f"{path}:{line_number}: row width does not match header")
            continue
        row.update({field: row[field].strip() for field in fields})
    return rows


def load_json(path: Path, errors: list[str]) -> dict[str, str]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        errors.append(f"{path}: cannot read JSON: {exc}")
        return {}
    if not isinstance(value, dict):
        errors.append(f"{path}: localization root must be an object")
        return {}
    return {str(key): str(item) for key, item in value.items()}


def audit(root: Path) -> tuple[list[str], dict[str, int]]:
    errors: list[str] = []
    registry_rows = read_csv(
        root / "docs" / "ELEMENT_REGISTRY.csv",
        tuple(next(csv.reader((root / "docs" / "ELEMENT_REGISTRY.csv").open(
            "r", encoding="utf-8-sig", newline=""
        )))),
        errors,
    )
    policy_rows = read_csv(
        root / "docs" / "CONTENT_MENU_POLICY.csv", POLICY_FIELDS, errors
    )
    baseline_rows = read_csv(
        root / "tests" / "data" / "rc8_custom_special_menu_baseline.csv",
        BASELINE_FIELDS,
        errors,
    )
    if errors:
        return errors, {}

    registry_by_id: dict[int, dict[str, str]] = {}
    registry_by_identifier: dict[str, dict[str, str]] = {}
    for row in registry_rows:
        try:
            stable_id = int(row["stable_id"])
        except (KeyError, ValueError):
            errors.append(f"element registry: invalid stable_id for {row.get('identifier', '')}")
            continue
        registry_by_id[stable_id] = row
        registry_by_identifier[row["identifier"]] = row

    policy_by_id: dict[int, dict[str, str]] = {}
    policy_by_identifier: dict[str, dict[str, str]] = {}
    previous_id = -1
    for row in policy_rows:
        try:
            stable_id = int(row["stable_id"])
        except ValueError:
            errors.append(f"menu policy: invalid stable_id for {row['identifier']}")
            continue
        if stable_id <= previous_id:
            errors.append("menu policy: stable IDs must be strictly increasing")
        previous_id = stable_id
        if stable_id in policy_by_id or row["identifier"] in policy_by_identifier:
            errors.append(f"menu policy: duplicate {row['identifier']} or ID {stable_id}")
            continue
        policy_by_id[stable_id] = row
        policy_by_identifier[row["identifier"]] = row
        registry = registry_by_id.get(stable_id)
        if not registry or registry["identifier"] != row["identifier"]:
            errors.append(f"menu policy: registry mismatch for ID {stable_id}")
        allowed = EXPECTED_KIND_POLICY.get(row["content_kind"])
        if not allowed or row["main_menu_policy"] not in allowed:
            errors.append(
                f"menu policy: invalid kind/policy pair for {row['identifier']}: "
                f"{row['content_kind']}/{row['main_menu_policy']}"
            )
        if row["content_kind"] == "custom_special" and row["main_menu_policy"] != "preserve_existing":
            errors.append(f"menu policy: custom special moved: {row['identifier']}")

    expected_ids = {
        stable_id for stable_id in registry_by_id
        if 196 <= stable_id <= 685
    }
    if set(policy_by_id) != expected_ids:
        missing = sorted(expected_ids - set(policy_by_id))
        extra = sorted(set(policy_by_id) - expected_ids)
        errors.append(f"menu policy coverage mismatch: missing={missing[:8]} extra={extra[:8]}")
    if any(stable_id <= 195 for stable_id in policy_by_id):
        errors.append("menu policy must not override official element IDs")

    periodic_rows = read_csv(
        root / "docs" / "PERIODIC_ELEMENT_SOURCE_MAP.csv",
        (
            "atomic_number", "symbol", "zh_name", "en_name", "official_mapping",
            "omnipack_mapping", "candidate_mods", "selected_source",
            "selected_source_commit", "implementation_type", "stable_id", "status",
            "tests",
        ),
        errors,
    )
    for row in periodic_rows:
        identifier = row["official_mapping"] or row["omnipack_mapping"]
        stable_id = int(row["stable_id"])
        if stable_id <= 195:
            if stable_id in policy_by_id:
                errors.append(f"official periodic mapping overridden: {identifier}")
            continue
        policy = policy_by_identifier.get(identifier)
        if not policy or policy["content_kind"] != "periodic_element" or policy["main_menu_policy"] != "periodic_only":
            errors.append(f"periodic element lacks periodic-only policy: {identifier}")
        elif policy["periodic_atomic_numbers"] != row["atomic_number"]:
            errors.append(f"periodic element atomic-number mismatch: {identifier}")

    isotope_rows = read_csv(
        root / "docs" / "ISOTOPE_REGISTRY.csv",
        (
            "isotope_id", "atomic_number", "symbol", "zh_name", "en_name",
            "stable_id", "identifier", "half_life_model", "decay_type",
            "decay_product", "heat", "particles", "neutron_behavior", "status",
            "tests", "notes",
        ),
        errors,
    )
    for row in isotope_rows:
        if row["status"] != "implemented":
            continue
        policy = policy_by_identifier.get(row["identifier"])
        if not policy or policy["content_kind"] != "isotope" or policy["main_menu_policy"] != "periodic_only":
            errors.append(f"isotope lacks periodic-only policy: {row['identifier']}")
        elif policy["periodic_atomic_numbers"] != row["atomic_number"]:
            errors.append(f"isotope atomic-number mismatch: {row['identifier']}")

    links_path = root / "docs" / "PERIODIC_CONTENT_LINKS.csv"
    try:
        with links_path.open("r", encoding="utf-8-sig", newline="") as stream:
            link_rows = list(csv.DictReader(stream))
    except (OSError, csv.Error) as exc:
        errors.append(f"{links_path}: cannot read: {exc}")
        link_rows = []
    linked = {row.get("tool_identifier", "") for row in link_rows}
    for row in policy_rows:
        kind = row["content_kind"]
        if row["main_menu_policy"] == "periodic_only" and row["identifier"] not in linked:
            errors.append(f"periodic-only content has no picker link: {row['identifier']}")
        if kind in {"organic_material", "alloy_engineering", "custom_special"} and row["identifier"] in linked:
            errors.append(f"periodic table wrongly includes {kind}: {row['identifier']}")

    expected_organic = {
        "OMNI_PT_CH4M", "OMNI_PT_ETHL", "OMNI_PT_ACET", "OMNI_PT_BENZ",
        "OMNI_PT_POLY", "OMNI_PT_RUBR",
    }
    for identifier in expected_organic:
        row = policy_by_identifier.get(identifier)
        if not row or row["content_kind"] != "organic_material" or row["main_menu_policy"] != "organic_menu":
            errors.append(f"organic menu smoke item misplaced: {identifier}")
    expected_alloys = {
        "OMNI_PT_STEL", "OMNI_PT_SSIL", "OMNI_PT_BRNZ", "OMNI_PT_BRAS",
        "OMNI_PT_NCRM", "OMNI_PT_TIAL",
    }
    for identifier in expected_alloys:
        row = policy_by_identifier.get(identifier)
        if not row or row["main_menu_policy"] != "alloy_menu":
            errors.append(f"alloy menu smoke item misplaced: {identifier}")

    baseline_identifiers: set[str] = set()
    for row in baseline_rows:
        identifier = row["identifier"]
        baseline_identifiers.add(identifier)
        policy = policy_by_identifier.get(identifier)
        registry = registry_by_identifier.get(identifier)
        if not policy or policy["content_kind"] != "custom_special" or policy["main_menu_policy"] != "preserve_existing":
            errors.append(f"rc8 custom special policy changed: {identifier}")
            continue
        if not registry:
            errors.append(f"rc8 custom special missing from registry: {identifier}")
            continue
        if registry["stable_id"] != row["stable_id"] or registry["menu_category"] != row["menu_section"]:
            errors.append(f"rc8 custom special registry position changed: {identifier}")
        source_path = root / registry["source_file"]
        try:
            source = source_path.read_text(encoding="utf-8")
        except OSError as exc:
            errors.append(f"{source_path}: cannot read custom-special source: {exc}")
            continue
        if not re.search(rf"\bMenuSection\s*=\s*{re.escape(row['menu_section'])}\s*;", source):
            errors.append(f"rc8 custom special MenuSection changed: {identifier}")
        if not re.search(rf"\bMenuVisible\s*=\s*{re.escape(row['menu_visible'])}\s*;", source):
            errors.append(f"rc8 custom special MenuVisible changed: {identifier}")
    expected_custom = {
        row["identifier"] for row in policy_rows
        if row["content_kind"] == "custom_special"
    }
    if baseline_identifiers != expected_custom:
        errors.append("rc8 custom-special baseline and policy registry disagree")

    menu_header = (root / "src" / "simulation" / "MenuSection.h").read_text(encoding="utf-8")
    expected_official_constants = {
        "SC_WALL": 0, "SC_ELEC": 1, "SC_POWERED": 2, "SC_SENSOR": 3,
        "SC_FORCE": 4, "SC_EXPLOSIVE": 5, "SC_GAS": 6, "SC_LIQUID": 7,
        "SC_POWDERS": 8, "SC_SOLIDS": 9, "SC_NUCLEAR": 10,
        "SC_SPECIAL": 11, "SC_LIFE": 12, "SC_TOOL": 13,
        "SC_FAVORITES": 14, "SC_DECO": 15,
    }
    for name, value in expected_official_constants.items():
        if not re.search(rf"constexpr int {name}\s*=\s*{value}\s*;", menu_header):
            errors.append(f"official menu constant moved: {name}")
    for name in ("SC_OMNI_ORGANIC", "SC_OMNI_ALLOY"):
        if not re.search(rf"constexpr int {name}\s*=", menu_header):
            errors.append(f"missing menu constant: {name}")
    repository_text = "\n".join(
        path.read_text(encoding="utf-8", errors="replace")
        for path in (
            root / "src" / "simulation" / "MenuSection.h",
            root / "src" / "simulation" / "SimulationData.cpp",
        )
    )
    if "SC_OMNI_SPECIAL" in repository_text or "sim.menu.omni_special" in repository_text:
        errors.append("forbidden Omni Specials menu was added")

    simulation_data = (root / "src" / "simulation" / "SimulationData.cpp").read_text(encoding="utf-8")
    for key in ("sim.menu.omni_organic", "sim.menu.omni_alloy"):
        if f'String("{key}")' not in simulation_data:
            errors.append(f"menu not registered in SimulationData: {key}")
    game_model = (root / "src" / "gui" / "game" / "GameModel.cpp").read_text(encoding="utf-8")
    if "GetOmniStandardMenuSection" not in game_model:
        errors.append("GameModel does not apply the centralized presentation policy")
    if "menuList[SC_FAVORITES]->AddTool(tool)" not in game_model:
        errors.append("favorites no longer accept hidden periodic tools")

    en = load_json(root / "src" / "lang" / "en-US.json", errors)
    zh = load_json(root / "src" / "lang" / "zh-CN.json", errors)
    expected_localization = {
        "sim.menu.omni_organic": ("Organic Materials", "有机材料"),
        "sim.menu.omni_alloy": ("Alloys & Engineering", "合金与工程材料"),
    }
    for key, (english, chinese) in expected_localization.items():
        if en.get(key) != english or zh.get(key) != chinese:
            errors.append(f"menu localization mismatch: {key}")
    for catalog in (en, zh):
        for key, value in catalog.items():
            if key.startswith("periodic.") and "identifier" in value.casefold():
                errors.append(f"developer term exposed by {key}")
    encyclopedia_source = (
        root / "src" / "gui" / "elementsearch" / "ElementSearchActivity.cpp"
    ).read_text(encoding="utf-8")
    if 'Tr("encyclopedia.identifier")' in encyclopedia_source:
        errors.append("element encyclopedia still exposes the internal identifier field")

    stats = {
        "policy_rows": len(policy_rows),
        "periodic_only": sum(row["main_menu_policy"] == "periodic_only" for row in policy_rows),
        "organic_menu": sum(row["main_menu_policy"] == "organic_menu" for row in policy_rows),
        "alloy_menu": sum(row["main_menu_policy"] == "alloy_menu" for row in policy_rows),
        "custom_special_preserved": len(expected_custom),
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
        print(f"content-menu-policy: FAIL errors={len(errors)}")
        return 1
    if not args.quiet:
        print("content-menu-policy: PASS " + " ".join(f"{key}={value}" for key, value in stats.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
