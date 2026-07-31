#!/usr/bin/env python3
"""Fail-closed validation for generated mod source and element catalogs."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
from typing import Sequence

from catalog_common import SOURCE_CATALOG_COLUMNS, SOURCE_TYPES
from generate_mod_report import ELEMENT_COLUMNS


COMMIT = re.compile(r"^[0-9a-f]{40}$")
CONTENT_DIGEST = re.compile(r"^sha256:[0-9A-F]{64}$")
URL = re.compile(r"^https://(?:github\.com|powdertoy\.co\.uk|starcatcher\.us)/")
HTTPS_URL = re.compile(r"^https://")
IDENTIFIER = re.compile(r"^[A-Z][A-Z0-9_]{2,127}$")
DECISIONS = {"A_direct_port", "B_rewrite_port", "C_reference_only", "D_reject"}
RISKS = {"low", "medium", "high", "review", "unknown"}
AUDIT_STATUSES = {
    "scanned", "downloaded_source", "not_cloned", "empty_repository",
    "checkout_incomplete", "checksum_mismatch",
}


def read_csv(path: Path, expected: tuple[str, ...], errors: list[str]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != expected:
                errors.append(f"{path}: unexpected columns or order")
                return []
            rows = list(reader)
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        errors.append(f"{path}: cannot read CSV: {exc}")
        return []
    for number, row in enumerate(rows, 2):
        if None in row or any(value is None for value in row.values()):
            errors.append(f"{path}:{number}: malformed row width")
    return rows


def validate(root: Path) -> list[str]:
    errors: list[str] = []
    sources_path = root / "docs/MOD_SOURCE_CATALOG.csv"
    elements_path = root / "docs/MOD_ELEMENT_CATALOG.csv"
    sources = read_csv(sources_path, SOURCE_CATALOG_COLUMNS, errors)
    elements = read_csv(elements_path, ELEMENT_COLUMNS, errors)
    mod_ids: set[str] = set()
    for number, row in enumerate(sources, 2):
        mod_id = row["mod_id"]
        if not re.fullmatch(r"[a-z][a-z0-9_]*", mod_id) or mod_id in mod_ids:
            errors.append(f"{sources_path}:{number}: invalid or duplicate mod_id {mod_id!r}")
        mod_ids.add(mod_id)
        if row["source_type"] not in SOURCE_TYPES:
            errors.append(f"{sources_path}:{number}: invalid source_type {row['source_type']!r}")
        if row["repository_url"] and not URL.match(row["repository_url"]):
            errors.append(f"{sources_path}:{number}: invalid repository_url")
        if row["forum_url"] and not URL.match(row["forum_url"]):
            errors.append(f"{sources_path}:{number}: invalid forum_url")
        if row["download_url"] and not HTTPS_URL.match(row["download_url"]):
            errors.append(f"{sources_path}:{number}: invalid download_url")
        if row["license_verified"] not in {"true", "false"}:
            errors.append(f"{sources_path}:{number}: license_verified must be true or false")
        if row["audit_status"] == "scanned" and not COMMIT.fullmatch(row["source_commit"]):
            errors.append(f"{sources_path}:{number}: scanned source lacks a full commit")
        if row["audit_status"] == "downloaded_source" and not CONTENT_DIGEST.fullmatch(row["source_commit"]):
            errors.append(f"{sources_path}:{number}: downloaded source lacks a SHA-256 digest")
        if row["audit_status"] not in AUDIT_STATUSES:
            errors.append(f"{sources_path}:{number}: invalid audit_status {row['audit_status']!r}")
        if row["license_verified"] == "true" and row["license"] in {"", "unknown", "unclassified"}:
            errors.append(f"{sources_path}:{number}: verified license has no classified value")
        if row["binary_only"] not in {"true", "false"}:
            errors.append(f"{sources_path}:{number}: binary_only must be true or false")

    element_keys: set[tuple[str, str, str]] = set()
    for number, row in enumerate(elements, 2):
        if row["source_mod"] not in mod_ids:
            errors.append(f"{elements_path}:{number}: unknown source_mod {row['source_mod']!r}")
        if not IDENTIFIER.fullmatch(row["source_identifier"]):
            errors.append(f"{elements_path}:{number}: invalid source_identifier {row['source_identifier']!r}")
        key = (row["source_mod"], row["source_file"], row["source_identifier"])
        if key in element_keys:
            errors.append(f"{elements_path}:{number}: duplicate source element key")
        element_keys.add(key)
        if row["port_decision"] not in DECISIONS:
            errors.append(f"{elements_path}:{number}: invalid port_decision")
        if row["performance_risk"] not in RISKS or row["save_risk"] not in RISKS:
            errors.append(f"{elements_path}:{number}: invalid risk value")
        if row["global_scan"] not in {"true", "false"}:
            errors.append(f"{elements_path}:{number}: global_scan must be true or false")
        for field in ("reaction_count", "particle_creation", "particle_deletion"):
            if not row[field].isdigit():
                errors.append(f"{elements_path}:{number}: {field} must be a nonnegative integer")

    required_reports = (
        "MOD_REPOSITORY_AUDIT.md", "MOD_DUPLICATE_REPORT.md", "MOD_PORTING_PLAN.md",
        "MOD_LICENSE_AUDIT.md", "MOD_REJECTION_LOG.md", "MOD_EXTRACTION_REPORT.md",
        "MOD_FEATURE_CATALOG.csv",
    )
    for name in required_reports:
        path = root / "docs" / name
        if not path.is_file() or path.stat().st_size == 0:
            errors.append(f"{path}: required generated report is missing or empty")
    return errors


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    errors = validate(args.source_root.resolve())
    if errors:
        for error in errors:
            print(f"ERROR {error}", file=sys.stderr)
        return 1
    if not args.quiet:
        print("validate-mod-catalog: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
