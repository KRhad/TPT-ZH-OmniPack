#!/usr/bin/env python3
"""Generate the compiled OmniPack content-presentation policy table."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
from typing import Sequence


FIELDS = (
    "identifier",
    "stable_id",
    "content_kind",
    "main_menu_policy",
    "periodic_atomic_numbers",
    "standalone_menu",
)
KIND_TOKENS = {
    "periodic_element": "PeriodicElement",
    "isotope": "Isotope",
    "inorganic_compound": "InorganicCompound",
    "organic_material": "OrganicMaterial",
    "alloy_engineering": "AlloyEngineering",
    "ecology_material": "EcologyMaterial",
    "nuclear_device": "NuclearDevice",
    "custom_special": "CustomSpecial",
    "official": "Official",
    "compatibility_alias": "CompatibilityAlias",
    "hidden_internal": "HiddenInternal",
}
POLICY_TOKENS = {
    "official_default": "OfficialDefault",
    "periodic_only": "PeriodicOnly",
    "organic_menu": "OrganicMenu",
    "alloy_menu": "AlloyMenu",
    "existing_dedicated_menu": "ExistingDedicatedMenu",
    "preserve_existing": "PreserveExisting",
    "hidden": "Hidden",
}
IDENTIFIER = re.compile(r"[A-Z][A-Z0-9_]*")
ATOMIC_NUMBERS = re.compile(r"(?:[1-9][0-9]*)(?:\|[1-9][0-9]*)*")


def cpp_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != FIELDS:
            raise ValueError(f"unexpected columns in {path}: {reader.fieldnames}")
        rows = list(reader)
    seen_identifiers: set[str] = set()
    seen_ids: set[int] = set()
    previous_id = -1
    for line_number, row in enumerate(rows, 2):
        if None in row or any(row[field] is None for field in FIELDS):
            raise ValueError(f"{path}:{line_number}: row width does not match header")
        row.update({field: row[field].strip() for field in FIELDS})
        identifier = row["identifier"]
        if not IDENTIFIER.fullmatch(identifier):
            raise ValueError(f"{path}:{line_number}: invalid identifier {identifier!r}")
        if identifier in seen_identifiers:
            raise ValueError(f"{path}:{line_number}: duplicate identifier {identifier}")
        seen_identifiers.add(identifier)
        try:
            stable_id = int(row["stable_id"])
        except ValueError as exc:
            raise ValueError(f"{path}:{line_number}: invalid stable_id") from exc
        if not 0 <= stable_id < 1024 or stable_id in seen_ids:
            raise ValueError(f"{path}:{line_number}: duplicate or invalid stable_id {stable_id}")
        if stable_id <= previous_id:
            raise ValueError(f"{path}:{line_number}: stable IDs must be strictly increasing")
        previous_id = stable_id
        seen_ids.add(stable_id)
        if row["content_kind"] not in KIND_TOKENS:
            raise ValueError(f"{path}:{line_number}: invalid content_kind")
        if row["main_menu_policy"] not in POLICY_TOKENS:
            raise ValueError(f"{path}:{line_number}: invalid main_menu_policy")
        atoms = row["periodic_atomic_numbers"]
        if atoms and not ATOMIC_NUMBERS.fullmatch(atoms):
            raise ValueError(f"{path}:{line_number}: invalid periodic_atomic_numbers")
        if atoms:
            values = [int(value) for value in atoms.split("|")]
            if values != sorted(set(values)) or not all(1 <= value <= 118 for value in values):
                raise ValueError(f"{path}:{line_number}: non-canonical atomic-number list")
    return rows


def render(path: Path) -> str:
    rows = read_rows(path)
    entries = []
    for row in rows:
        entries.append(
            "\t{ "
            + ", ".join(
                (
                    cpp_string(row["identifier"]),
                    row["stable_id"],
                    f"OmniContentKind::{KIND_TOKENS[row['content_kind']]}",
                    f"OmniMainMenuPolicy::{POLICY_TOKENS[row['main_menu_policy']]}",
                    cpp_string(row["periodic_atomic_numbers"]),
                    cpp_string(row["standalone_menu"]),
                )
            )
            + " },"
        )
    return "\n".join(
        (
            '// Generated from docs/CONTENT_MENU_POLICY.csv. Do not edit.',
            '#include "simulation/OmniContentPresentation.h"',
            "",
            "#include <array>",
            "",
            "namespace",
            "{",
            f"constexpr std::array<OmniContentPresentationRecord, {len(rows)}> records{{{{",
            *entries,
            "}};",
            "}",
            "",
            "std::span<OmniContentPresentationRecord const> GetOmniContentPresentationData()",
            "{",
            "\treturn records;",
            "}",
            "",
        )
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("policy", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    rendered = render(args.policy)
    if args.check:
        if not args.output.is_file() or args.output.read_text(encoding="utf-8") != rendered:
            print(f"ERROR {args.output}: generated menu policy data is stale")
            return 1
        print(f"content-menu-policy-data: PASS rows={len(read_rows(args.policy))}")
        return 0
    args.output.write_text(rendered, encoding="utf-8", newline="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
