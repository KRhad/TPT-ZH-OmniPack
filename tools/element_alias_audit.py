#!/usr/bin/env python3
"""Fail-closed audit for merged element compatibility aliases."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import sys
from typing import Sequence


EXPECTED_COLUMNS = (
    "alias_identifier",
    "alias_stable_id",
    "canonical_identifier",
    "canonical_stable_id",
    "classification",
    "menu_policy",
    "creation_policy",
    "load_policy",
    "save_policy",
    "indirect_type_policy",
    "stable_id_policy",
    "test_status",
    "notes",
)


def read_csv(path: Path) -> tuple[list[dict[str, str]], list[str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != EXPECTED_COLUMNS:
                return [], [f"{path}: header mismatch"]
            rows = list(reader)
    except (OSError, csv.Error) as exc:
        return [], [f"{path}: cannot read CSV: {exc}"]
    errors: list[str] = []
    for number, row in enumerate(rows, start=2):
        if None in row:
            errors.append(f"{path}:{number}: extra columns")
        for field in EXPECTED_COLUMNS:
            value = row.get(field)
            if value is None or not value.strip() or value != value.strip():
                errors.append(f"{path}:{number}: invalid {field}")
    return rows, errors


def read_registry(path: Path) -> tuple[dict[str, dict[str, str]], list[str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, csv.Error) as exc:
        return {}, [f"{path}: cannot read registry: {exc}"]
    by_identifier: dict[str, dict[str, str]] = {}
    errors: list[str] = []
    for number, row in enumerate(rows, start=2):
        identifier = row.get("identifier", "")
        if identifier in by_identifier:
            errors.append(f"{path}:{number}: duplicate identifier {identifier}")
        by_identifier[identifier] = row
    return by_identifier, errors


def read_text(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        errors.append(f"{path}: cannot read UTF-8 text: {exc}")
        return ""


def audit(root: Path) -> list[str]:
    docs = root / "docs"
    rows, errors = read_csv(docs / "ELEMENT_ALIAS_REGISTRY.csv")
    registry, registry_errors = read_registry(docs / "ELEMENT_REGISTRY.csv")
    errors.extend(registry_errors)
    if not rows:
        errors.append("ELEMENT_ALIAS_REGISTRY.csv: at least one audited alias is required")
        return errors

    seen_aliases: set[str] = set()
    seen_ids: set[int] = set()
    for number, row in enumerate(rows, start=2):
        alias = row["alias_identifier"]
        canonical = row["canonical_identifier"]
        try:
            alias_id = int(row["alias_stable_id"])
            canonical_id = int(row["canonical_stable_id"])
        except ValueError:
            errors.append(f"ELEMENT_ALIAS_REGISTRY.csv:{number}: non-integer stable ID")
            continue
        if alias in seen_aliases or alias_id in seen_ids:
            errors.append(f"ELEMENT_ALIAS_REGISTRY.csv:{number}: duplicate alias")
        seen_aliases.add(alias)
        seen_ids.add(alias_id)
        expected_policy = {
            "classification": "complete_behavior_duplicate",
            "menu_policy": "hidden",
            "creation_policy": "blocked",
            "load_policy": "preserve_then_migrate",
            "save_policy": "canonical_after_migration",
            "indirect_type_policy": "legacy_alias_resolves",
            "stable_id_policy": "never_reuse",
            "test_status": "runtime-tested",
        }
        for field, expected in expected_policy.items():
            if row[field] != expected:
                errors.append(
                    f"ELEMENT_ALIAS_REGISTRY.csv:{number}: {field} must be {expected}"
                )
        alias_row = registry.get(alias)
        canonical_row = registry.get(canonical)
        if alias_row is None or canonical_row is None:
            errors.append(f"ELEMENT_ALIAS_REGISTRY.csv:{number}: registry target missing")
            continue
        expected_alias = {
            "stable_id": str(alias_id),
            "duplicate_of": canonical,
            "is_duplicate": "true",
            "implementation_status": "implemented",
            "save_compatibility": "migratable",
        }
        for field, expected in expected_alias.items():
            if alias_row.get(field) != expected:
                errors.append(f"{alias}: {field} must be {expected}")
        if canonical_row.get("stable_id") != str(canonical_id):
            errors.append(f"{canonical}: canonical stable ID mismatch")
        if canonical_row.get("is_duplicate") != "false":
            errors.append(f"{canonical}: canonical element cannot itself be a duplicate")

    mscr = read_text(root / "src/simulation/elements/MSCR.cpp", errors)
    metallurgy = read_text(root / "src/simulation/OmniMetallurgy.cpp", errors)
    brmt = read_text(root / "src/simulation/elements/BRMT.cpp", errors)
    simulation = read_text(root / "src/simulation/Simulation.cpp", errors)
    content = read_text(root / "src/gui/game/OmniContent.cpp", errors)
    periodic = read_text(root / "src/simulation/OmniPeriodic.cpp", errors)
    base = read_text(root / "src/simulation/elements/BASE.cpp", errors)
    runtime = read_text(root / "tools/runtime/element_alias_regression.lua", errors)

    required = {
        "MSCR hidden menu": (mscr, "MenuVisible = 0"),
        "MSCR migration update": (mscr, "OmniMetallurgyLegacyScrapAliasUpdate"),
        "official BRMT enhancement": (brmt, "OmniMetallurgyScrapUpdate"),
        "recoverable BRMT state isolation": (
            brmt,
            "if (IsOmniRecoverableScrap(parts[i]))\n\t\treturn OmniMetallurgyScrapUpdate",
        ),
        "recoverable marker": (metallurgy, "OmniRecoverableScrapMarker"),
        "legacy type migration": (metallurgy, "parts[i].type != PT_MSCR"),
        "source melting point": (metallurgy, "source.HighTemperature"),
        "generic transition guard": (simulation, "IsOmniRecoverableScrap(parts[i])"),
        "selection alias block": (content, "OmniSelectionRestriction::CompatibilityAlias"),
        "save module marker detection": (content, "IsOmniRecoverableScrap(particle)"),
        "legacy OPS migration regression": (runtime, "legacy-load-migrate-resave"),
        "canonical OPS restart regression": (runtime, "canonical-reload-verified"),
    }
    for label, (text, marker) in required.items():
        if marker not in text:
            errors.append(f"source contract missing: {label}")
    for label, text in (("periodic", periodic), ("official BASE", base)):
        if "PT_MSCR" in text:
            errors.append(f"{label}: new production still references legacy PT_MSCR")
        if "MarkOmniRecoverableScrap" not in text:
            errors.append(f"{label}: canonical BRMT production lacks recoverable marker")
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Audit merged element aliases.")
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
            print(f"element-alias-audit: ERROR {error}", file=sys.stderr)
        print(f"element-alias-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("element-alias-audit: PASS (1 compatibility alias, 1 canonical official element)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
