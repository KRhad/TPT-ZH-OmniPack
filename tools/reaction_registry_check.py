#!/usr/bin/env python3
"""Fail-closed static validation for the OmniPack reaction registry.

The registry is deliberately machine-readable.  Material expressions use
``;`` for simultaneous particles and ``|`` for alternatives.  A particle may
have a ``{qualifier}`` and/or a ``*count`` suffix.  The checker validates the
schema, current reaction coverage, element references, module budgets and test
evidence without modifying repository files.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence


REGISTRY_COLUMNS = (
    "reaction_id",
    "inputs",
    "outputs",
    "temperature",
    "pressure",
    "electricity",
    "catalyst",
    "neighborhood",
    "probability",
    "event_budget",
    "side_products",
    "failure_mode",
    "cleanup",
    "module",
    "tests",
)

MODULE_PREFIX = {
    "metallurgy": "metallurgy.",
    "biology": "biology.",
    "chemistry": "chemistry.",
    "advanced_nuclear": "nuclear.",
}

MODULE_BUDGETS = {
    "metallurgy": {"2048/frame", "unbudgeted-local-update"},
    "biology": {"1024/frame"},
    "chemistry": {"1536/frame"},
    "advanced_nuclear": {"512/frame"},
}

REQUIRED_REACTIONS = {
    "metallurgy.alloy_stainless",
    "metallurgy.alloy_tool_steel",
    "metallurgy.alloy_bronze",
    "metallurgy.alloy_brass",
    "metallurgy.alloy_nichrome",
    "metallurgy.alloy_aluminium_magnesium",
    "metallurgy.steelmaking",
    "metallurgy.pressure_scrap",
    "metallurgy.scrap_remelt",
    "metallurgy.copper_corrosion",
    "metallurgy.magnesium_oxidation",
    "metallurgy.zinc_sacrificial_protection",
    "metallurgy.zinc_corrosion",
    "metallurgy.nichrome_resistive_heating",
    "metallurgy.wood_charcoal",
    "metallurgy.coal_coke",
    "metallurgy.radiation_shield_assembly",
    "biology.algae_photosynthesis",
    "biology.mycelium_decomposition",
    "biology.spore_germination",
    "biology.pathogen_infection",
    "biology.sterilization",
    "biology.biofilm_filtration",
    "biology.humus_fertilizer_recovery",
    "chemistry.oil_cracking",
    "chemistry.kerosene_to_gasoline",
    "chemistry.kerosene_to_acetylene",
    "chemistry.acetylene_polymerisation",
    "chemistry.peroxide_decomposition",
    "chemistry.peroxide_pathogen_treatment",
    "chemistry.slag_acid_leaching",
    "chemistry.chlorine_hydrogen",
    "chemistry.ammonia_neutralisation",
    "chemistry.fertilizer_use",
    "chemistry.fermentation",
    "chemistry.ammonia_synthesis",
    "chemistry.peroxide_synthesis",
    "chemistry.water_electrolysis",
    "nuclear.fuel_fission",
    "nuclear.coolant_boil",
    "nuclear.waste_stabilization",
    "nuclear.shield_absorption",
    "nuclear.neutron_generation",
}

REACTION_ID = re.compile(r"^[a-z][a-z0-9_]*\.[a-z][a-z0-9_]*$")
PT_NAME = re.compile(r"\bPT_([A-Z0-9]+)\b")
MATERIAL_EXPRESSION = re.compile(r"^[A-Za-z0-9_.*=;|{}+<>/\-]+$")
MATERIAL_TOKEN = re.compile(
    r"^PT_[A-Z0-9]+(?:\{[A-Za-z0-9_.*=+<>/\-]+\})?(?:\*[1-9][0-9]*)?$"
)
FORBIDDEN_CONTROL = re.compile(r"[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]")
PLACEHOLDERS = {"", "none", "n/a", "na", "tbd", "todo", "unknown", "not_tested"}


def _read_csv(path: Path, errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != REGISTRY_COLUMNS:
                errors.append(
                    f"{path}: expected columns in this order: {','.join(REGISTRY_COLUMNS)}"
                )
                return []
            rows = list(reader)
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read UTF-8 CSV: {exc}")
        return []
    for row_number, row in enumerate(rows, start=2):
        if None in row:
            errors.append(f"{path}:{row_number}: row contains fields beyond the declared schema")
    return rows


def _known_element_names(path: Path, errors: list[str]) -> set[str]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read element registry: {exc}")
        return set()
    names = {
        row.get("meson_name", "")
        for row in rows
        if re.fullmatch(r"[A-Z][A-Z0-9_]*", row.get("meson_name", ""))
    }
    if not names:
        errors.append(f"{path}: no canonical element names found")
    return names


def _check_material_expression(
    value: str,
    field: str,
    row_number: int,
    path: Path,
    known_elements: set[str],
    errors: list[str],
    *,
    allow_none: bool = False,
) -> None:
    if value == "none" and allow_none:
        return
    if not value or not MATERIAL_EXPRESSION.fullmatch(value):
        errors.append(f"{path}:{row_number}: {field} has invalid material expression {value!r}")
        return
    if value.count("{") != value.count("}"):
        errors.append(f"{path}:{row_number}: {field} has unbalanced qualifiers")
    tokens = [token for group in value.split(";") for token in group.split("|")]
    invalid_tokens = [token for token in tokens if not MATERIAL_TOKEN.fullmatch(token)]
    if invalid_tokens:
        errors.append(
            f"{path}:{row_number}: {field} contains invalid material tokens {invalid_tokens}"
        )
    names = PT_NAME.findall(value)
    if not names:
        errors.append(f"{path}:{row_number}: {field} must reference at least one PT_* element")
        return
    unknown = sorted(set(names) - known_elements)
    if unknown:
        errors.append(f"{path}:{row_number}: {field} references unknown elements {unknown}")


def _check_test_paths(
    value: str, row_number: int, path: Path, root: Path, errors: list[str]
) -> None:
    entries = value.split(";") if value else []
    if not entries or any(not entry for entry in entries):
        errors.append(f"{path}:{row_number}: tests must contain repository-relative evidence paths")
        return
    for entry in entries:
        candidate = Path(entry)
        if (
            candidate.is_absolute()
            or "\\" in entry
            or ".." in candidate.parts
            or not entry.startswith("tools/")
        ):
            errors.append(f"{path}:{row_number}: unsafe test path {entry!r}")
            continue
        if not (root / candidate).is_file():
            errors.append(f"{path}:{row_number}: test evidence does not exist: {entry}")


def _valid_temperature(value: str) -> bool:
    if value in {"any", ">=source.HighTemperature"}:
        return True
    comparison = r"(?:PT_[A-Z0-9]+|catalyst)?(?:>=|>)[0-9]+(?:\.[0-9]+)?K"
    temperature_range = r"(?:catalyst=)?\[[0-9]+(?:\.[0-9]+)?K;[0-9]+(?:\.[0-9]+)?K[\]\)]"
    if re.fullmatch(comparison, value) or re.fullmatch(temperature_range, value):
        return True
    return all(re.fullmatch(comparison, item) for item in value.split(";"))


def audit(root: Path, registry_path: Path | None = None) -> list[str]:
    root = root.resolve()
    path = registry_path or root / "docs" / "REACTION_REGISTRY.csv"
    errors: list[str] = []
    rows = _read_csv(path, errors)
    known_elements = _known_element_names(root / "docs" / "ELEMENT_REGISTRY.csv", errors)
    if not rows or not known_elements:
        return errors

    seen: dict[str, int] = {}
    for row_number, row in enumerate(rows, start=2):
        for field in REGISTRY_COLUMNS:
            value = row.get(field)
            if value is None:
                errors.append(f"{path}:{row_number}: missing field {field}")
                continue
            if value != value.strip():
                errors.append(f"{path}:{row_number}: {field} has leading or trailing whitespace")
            if FORBIDDEN_CONTROL.search(value):
                errors.append(f"{path}:{row_number}: {field} contains a control character")

        reaction_id = row.get("reaction_id", "")
        if not REACTION_ID.fullmatch(reaction_id):
            errors.append(f"{path}:{row_number}: invalid reaction_id {reaction_id!r}")
        if reaction_id in seen:
            errors.append(
                f"{path}:{row_number}: duplicate reaction_id {reaction_id!r}; first at row {seen[reaction_id]}"
            )
        else:
            seen[reaction_id] = row_number

        module = row.get("module", "")
        prefix = MODULE_PREFIX.get(module)
        if prefix is None:
            errors.append(f"{path}:{row_number}: invalid module {module!r}")
        elif reaction_id and not reaction_id.startswith(prefix):
            errors.append(
                f"{path}:{row_number}: reaction_id {reaction_id!r} does not match module {module!r}"
            )

        _check_material_expression(
            row.get("inputs", ""), "inputs", row_number, path, known_elements, errors
        )
        _check_material_expression(
            row.get("outputs", ""), "outputs", row_number, path, known_elements, errors
        )
        _check_material_expression(
            row.get("catalyst", ""),
            "catalyst",
            row_number,
            path,
            known_elements,
            errors,
            allow_none=True,
        )
        _check_material_expression(
            row.get("side_products", ""),
            "side_products",
            row_number,
            path,
            known_elements,
            errors,
            allow_none=True,
        )

        if row.get("neighborhood") not in {"self", "3x3-local"}:
            errors.append(f"{path}:{row_number}: neighborhood must be 'self' or '3x3-local'")
        budget = row.get("event_budget", "")
        if budget not in MODULE_BUDGETS.get(module, set()):
            errors.append(
                f"{path}:{row_number}: event_budget {budget!r} is invalid for module {module!r}"
            )
        if row.get("electricity") not in {
            "none",
            "SPRK(ctype=PT_CATA)",
            "SPRK(ctype=PT_NGEN)",
            "SPRK(ctype=PT_NCRM)",
        }:
            errors.append(f"{path}:{row_number}: invalid electricity condition")
        if row.get("pressure") not in {"any", ">=2.0", "abs>=element-specific-threshold"}:
            errors.append(f"{path}:{row_number}: invalid pressure condition")
        temperature = row.get("temperature", "")
        if not _valid_temperature(temperature):
            errors.append(f"{path}:{row_number}: invalid temperature condition")
        probability = row.get("probability", "")
        if probability != "deterministic":
            fractions = re.findall(r"\b([0-9]+)/([0-9]+)\b", probability)
            invalid_fraction = any(
                int(denominator) == 0 or int(numerator) > int(denominator)
                for numerator, denominator in fractions
            )
            if (
                not fractions
                or invalid_fraction
                or not re.fullmatch(r"[A-Za-z0-9_=;/.>\-]+", probability)
            ):
                errors.append(f"{path}:{row_number}: invalid probability {probability!r}")

        for field in ("failure_mode", "cleanup"):
            value = row.get(field, "")
            if value.casefold() in PLACEHOLDERS or len(value) < 12:
                errors.append(f"{path}:{row_number}: {field} must be a concrete non-placeholder description")
        _check_test_paths(row.get("tests", ""), row_number, path, root, errors)

    missing = sorted(REQUIRED_REACTIONS - set(seen))
    extra = sorted(set(seen) - REQUIRED_REACTIONS)
    if missing:
        errors.append(f"{path}: missing current source reactions: {missing}")
    if extra:
        errors.append(f"{path}: unreviewed reaction IDs require checker update: {extra}")
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    parser.add_argument("--registry", type=Path)
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    root = args.source_root.resolve()
    registry = args.registry
    if registry is not None and not registry.is_absolute():
        registry = root / registry
    errors = audit(root, registry)
    if errors:
        for error in errors:
            print(f"reaction-registry-check: ERROR {error}", file=sys.stderr)
        print(f"reaction-registry-check: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print(f"reaction-registry-check: PASS ({len(REQUIRED_REACTIONS)} source-confirmed reactions)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
