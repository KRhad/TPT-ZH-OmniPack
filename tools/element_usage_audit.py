#!/usr/bin/env python3
"""Fail-closed audit for the 0.2 OmniPack element usage matrix."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import sys
from typing import Sequence


EXPECTED_COLUMNS = (
    "identifier",
    "stable_id",
    "code",
    "module",
    "production_status",
    "primary_use",
    "secondary_use",
    "consumption_path",
    "side_products",
    "hazards",
    "controls",
    "recovery_path",
    "tutorial_scenario",
    "stress_risk",
    "gap_codes",
    "disposition",
)

PRODUCTION_STATUSES = {
    "direct_only",
    "produced",
    "produced_or_direct",
    "byproduct",
    "recycled_or_direct",
}

GAP_CODES = {
    "none",
    "endpoint_byproduct",
    "invalid_type_fallback",
    "missing_cross_module_link",
    "missing_production",
    "missing_recovery",
    "missing_secondary_use",
    "no_machine_role",
    "single_use",
    "unverified_gameplay_claim",
}

DISPOSITIONS = {
    "keep",
    "link_in_0.2",
    "fix_or_test_claim",
    "add_recovery",
}


def read_csv(path: Path) -> tuple[list[dict[str, str]], list[str]]:
    errors: list[str] = []
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != EXPECTED_COLUMNS:
                errors.append(
                    f"{path.name} header mismatch: expected {','.join(EXPECTED_COLUMNS)}"
                )
                return [], errors
            rows = list(reader)
    except (OSError, csv.Error) as exc:
        return [], [f"cannot read {path}: {exc}"]
    for number, row in enumerate(rows, start=2):
        if None in row:
            errors.append(f"{path.name}:{number} has extra columns")
        for column in EXPECTED_COLUMNS:
            value = row.get(column)
            if value is None or not value.strip() or value != value.strip():
                errors.append(f"{path.name}:{number} has invalid {column}")
    return rows, errors


def read_registry(path: Path) -> tuple[dict[str, dict[str, str]], list[str]]:
    errors: list[str] = []
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except (OSError, csv.Error) as exc:
        return {}, [f"cannot read {path}: {exc}"]
    implemented: dict[str, dict[str, str]] = {}
    for row in rows:
        identifier = row.get("identifier", "")
        if identifier.startswith("OMNI_PT_") and row.get("implementation_status") == "implemented":
            if identifier in implemented:
                errors.append(f"duplicate implemented registry identifier: {identifier}")
            implemented[identifier] = row
    return implemented, errors


def audit(root: Path) -> list[str]:
    docs = root / "docs"
    registry, errors = read_registry(docs / "ELEMENT_REGISTRY.csv")
    rows, usage_errors = read_csv(docs / "ELEMENT_USAGE_MATRIX.csv")
    errors.extend(usage_errors)
    if not rows:
        return errors

    seen: set[str] = set()
    for number, row in enumerate(rows, start=2):
        identifier = row["identifier"]
        if identifier in seen:
            errors.append(f"ELEMENT_USAGE_MATRIX.csv:{number} duplicates {identifier}")
            continue
        seen.add(identifier)
        source = registry.get(identifier)
        if source is None:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} has no implemented OmniPack element: {identifier}"
            )
            continue
        for field in ("stable_id", "code", "module"):
            if row[field] != source.get(field, ""):
                errors.append(
                    f"ELEMENT_USAGE_MATRIX.csv:{number} {field} mismatch for {identifier}"
                )
        if row["production_status"] not in PRODUCTION_STATUSES:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} invalid production_status for {identifier}"
            )
        if row["disposition"] not in DISPOSITIONS:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} invalid disposition for {identifier}"
            )
        gap_values = row["gap_codes"].split(";")
        unknown_gaps = sorted(set(gap_values) - GAP_CODES)
        if unknown_gaps:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} unknown gap_codes for {identifier}: "
                + ",".join(unknown_gaps)
            )
        if "none" in gap_values and len(gap_values) != 1:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} mixes none with real gaps for {identifier}"
            )
        if row["production_status"] == "direct_only" and "missing_production" not in gap_values:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} direct_only lacks missing_production for {identifier}"
            )
        if gap_values != ["none"] and row["disposition"] == "keep":
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} unresolved gaps use keep for {identifier}"
            )
        if row["disposition"] == "fix_or_test_claim" and "unverified_gameplay_claim" not in gap_values:
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} fix_or_test_claim lacks claim gap for {identifier}"
            )
        if row["disposition"] == "add_recovery" and not {
            "endpoint_byproduct",
            "missing_recovery",
        }.intersection(gap_values):
            errors.append(
                f"ELEMENT_USAGE_MATRIX.csv:{number} add_recovery lacks recovery gap for {identifier}"
            )

    missing = sorted(set(registry) - seen)
    extra = sorted(seen - set(registry))
    if missing:
        errors.append("missing implemented OmniPack elements: " + ",".join(missing))
    if extra:
        errors.append("unknown usage matrix elements: " + ",".join(extra))
    if len(rows) != 48:
        errors.append(f"usage matrix must contain exactly 48 rows, found {len(rows)}")
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit the complete 0.2 usage and gap disposition matrix."
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
            print(f"element-usage-audit: ERROR {error}", file=sys.stderr)
        print(f"element-usage-audit: FAIL ({len(errors)} errors)", file=sys.stderr)
        return 1
    if not args.quiet:
        print("element-usage-audit: PASS (48 implemented OmniPack elements)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
