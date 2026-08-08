#!/usr/bin/env python3
"""Generate the compiled periodic-table material-link registry."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Sequence


FIELDS = (
    "tool_identifier", "content_kind", "primary_atomic_number",
    "related_atomic_numbers", "compound_group", "display_order",
    "periodic_visible", "formula", "zh_name", "en_name",
)
RELATED_FIELDS = (
    "tool_identifier", "stable_id", "content_kind", "primary_atomic_number",
    "related_atomic_numbers", "compound_group", "display_order",
    "periodic_visible", "formula", "zh_name", "en_name", "relation_basis",
    "module", "source_reference", "confidence", "notes",
)
KIND_TOKENS = {
    "periodic_element": "PeriodicElement",
    "isotope": "Isotope",
    "inorganic_compound": "InorganicCompound",
    "related_material": "RelatedMaterial",
}
GROUP_TOKENS = {
    "element": "Element", "allotrope": "Allotrope", "isotope": "Isotope",
    "oxide": "Oxide",
    "hydroxide": "Hydroxide", "acid": "Acid", "base": "Base",
    "salt": "Salt", "halide": "Halide", "sulfide": "Sulfide",
    "nitride": "Nitride", "carbide": "Carbide", "hydride": "Hydride",
    "organic": "Organic", "polymer": "Polymer", "alloy": "Alloy",
    "mineral": "Mineral", "ceramic": "Ceramic", "glass": "Glass",
    "semiconductor": "Semiconductor", "composite": "Composite",
    "engineering": "Engineering", "other": "Other",
}


def cpp_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def parse_atoms(value: str, path: Path, line_number: int) -> list[int]:
    try:
        atoms = [int(item) for item in value.split("|")]
    except ValueError as exc:
        raise ValueError(f"{path}:{line_number}: invalid related atomic numbers") from exc
    if atoms != sorted(set(atoms)) or not atoms or not all(1 <= atom <= 118 for atom in atoms):
        raise ValueError(f"{path}:{line_number}: non-canonical related atomic numbers")
    return atoms


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != FIELDS:
            raise ValueError(f"unexpected columns in {path}: {reader.fieldnames}")
        rows = list(reader)
    seen: set[str] = set()
    for line_number, row in enumerate(rows, 2):
        if None in row or any(row[field] is None for field in FIELDS):
            raise ValueError(f"{path}:{line_number}: row width does not match header")
        row.update({field: row[field].strip() for field in FIELDS})
        identifier = row["tool_identifier"]
        if not identifier or identifier in seen:
            raise ValueError(f"{path}:{line_number}: empty or duplicate tool_identifier")
        seen.add(identifier)
        if row["content_kind"] not in KIND_TOKENS:
            raise ValueError(f"{path}:{line_number}: invalid content_kind")
        if row["compound_group"] not in GROUP_TOKENS:
            raise ValueError(f"{path}:{line_number}: invalid compound_group")
        atoms = parse_atoms(row["related_atomic_numbers"], path, line_number)
        try:
            primary = int(row["primary_atomic_number"])
            order = int(row["display_order"])
        except ValueError as exc:
            raise ValueError(f"{path}:{line_number}: invalid numeric field") from exc
        if primary not in atoms or order < 0:
            raise ValueError(f"{path}:{line_number}: invalid primary atom or display order")
        if row["periodic_visible"] not in ("true", "false"):
            raise ValueError(f"{path}:{line_number}: periodic_visible must be true or false")
        if not row["formula"] or not row["zh_name"] or not row["en_name"]:
            raise ValueError(f"{path}:{line_number}: display fields must not be empty")
    return rows


def read_registry(path: Path) -> dict[str, dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        rows = list(reader)
        fieldnames = set(reader.fieldnames or ())
    required = {
        "identifier", "stable_id", "implementation_status", "english_name",
        "chinese_name", "module",
    }
    if not required.issubset(fieldnames):
        raise ValueError(f"missing required columns in {path}")
    result: dict[str, dict[str, str]] = {}
    for line_number, row in enumerate(rows, 2):
        identifier = (row.get("identifier") or "").strip()
        if not identifier or identifier in result:
            raise ValueError(f"{path}:{line_number}: empty or duplicate identifier")
        result[identifier] = {
            field: (row.get(field) or "").strip() for field in required
        }
    return result


def read_related_rows(
    path: Path, registry: dict[str, dict[str, str]]
) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != RELATED_FIELDS:
            raise ValueError(f"unexpected columns in {path}: {reader.fieldnames}")
        rows = list(reader)
    seen: set[str] = set()
    seen_ids: set[int] = set()
    for line_number, row in enumerate(rows, 2):
        if None in row or any(row[field] is None for field in RELATED_FIELDS):
            raise ValueError(f"{path}:{line_number}: row width does not match header")
        row.update({field: row[field].strip() for field in RELATED_FIELDS})
        identifier = row["tool_identifier"]
        if not identifier or identifier in seen:
            raise ValueError(f"{path}:{line_number}: empty or duplicate tool_identifier")
        seen.add(identifier)
        if row["content_kind"] != "related_material":
            raise ValueError(f"{path}:{line_number}: content_kind must be related_material")
        if row["compound_group"] not in GROUP_TOKENS:
            raise ValueError(f"{path}:{line_number}: invalid compound_group")
        atoms = parse_atoms(row["related_atomic_numbers"], path, line_number)
        try:
            stable_id = int(row["stable_id"])
            primary = int(row["primary_atomic_number"])
            order = int(row["display_order"])
        except ValueError as exc:
            raise ValueError(f"{path}:{line_number}: invalid numeric field") from exc
        if stable_id in seen_ids or not 0 <= stable_id < 1024:
            raise ValueError(f"{path}:{line_number}: duplicate or invalid stable_id")
        seen_ids.add(stable_id)
        if primary not in atoms or order < 0:
            raise ValueError(f"{path}:{line_number}: invalid primary atom or display order")
        if row["periodic_visible"] != "true":
            raise ValueError(f"{path}:{line_number}: related material must be visible")
        for field in (
            "formula", "zh_name", "en_name", "relation_basis", "module",
            "source_reference", "confidence", "notes",
        ):
            if not row[field]:
                raise ValueError(f"{path}:{line_number}: empty required field {field}")
        if row["confidence"] not in {"high", "medium", "low"}:
            raise ValueError(f"{path}:{line_number}: invalid confidence")
        record = registry.get(identifier)
        if not record or record["implementation_status"] != "implemented":
            raise ValueError(f"{path}:{line_number}: unknown or unimplemented identifier")
        if int(record["stable_id"]) != stable_id:
            raise ValueError(f"{path}:{line_number}: stable ID does not match registry")
        if record["chinese_name"] != row["zh_name"] or record["english_name"] != row["en_name"]:
            raise ValueError(f"{path}:{line_number}: names do not match registry")
        if record["module"] != row["module"]:
            raise ValueError(f"{path}:{line_number}: module does not match registry")
    return rows


def masks(atoms: list[int]) -> tuple[int, int]:
    low = 0
    high = 0
    for atom in atoms:
        if atom <= 64:
            low |= 1 << (atom - 1)
        else:
            high |= 1 << (atom - 65)
    return low, high


def render(
    path: Path,
    related_path: Path | None = None,
    registry_path: Path | None = None,
) -> str:
    rows = read_rows(path)
    registry = read_registry(registry_path) if registry_path else {}
    if (related_path is None) != (registry_path is None):
        raise ValueError("related index and registry must be supplied together")
    related_rows = read_related_rows(related_path, registry) if related_path else []
    duplicates = sorted(
        {row["tool_identifier"] for row in rows}
        & {row["tool_identifier"] for row in related_rows}
    )
    if duplicates:
        raise ValueError(
            "duplicate identifiers across periodic indices: " + ", ".join(duplicates)
        )
    compiled_rows: list[dict[str, str]] = []
    for row in rows:
        compiled = dict(row)
        record = registry.get(row["tool_identifier"])
        if registry and (
            not record or record["implementation_status"] != "implemented"
        ):
            raise ValueError(
                "periodic link references unknown or unimplemented tool "
                + row["tool_identifier"]
            )
        compiled["stable_id"] = record["stable_id"] if record else "-1"
        compiled.update(
            relation_basis="",
            module=record["module"] if record else "",
            source_reference="",
            confidence="",
            notes="",
        )
        compiled_rows.append(compiled)
    compiled_rows.extend(related_rows)
    entries = []
    for row in compiled_rows:
        atoms = [int(item) for item in row["related_atomic_numbers"].split("|")]
        low, high = masks(atoms)
        entries.append(
            "\t{ "
            + ", ".join(
                (
                    cpp_string(row["tool_identifier"]),
                    row["stable_id"],
                    f"PeriodicContentKind::{KIND_TOKENS[row['content_kind']]}",
                    row["primary_atomic_number"],
                    f"UINT64_C(0x{low:016X})",
                    f"UINT64_C(0x{high:016X})",
                    f"PeriodicCompoundGroup::{GROUP_TOKENS[row['compound_group']]}",
                    row["display_order"],
                    row["periodic_visible"],
                    cpp_string(row["formula"]),
                    cpp_string(row["zh_name"]),
                    cpp_string(row["en_name"]),
                    cpp_string(row["relation_basis"]),
                    cpp_string(row["module"]),
                    cpp_string(row["source_reference"]),
                    cpp_string(row["confidence"]),
                    cpp_string(row["notes"]),
                )
            )
            + " },"
        )
    return "\n".join(
        (
            '// Generated from docs/PERIODIC_CONTENT_LINKS.csv. Do not edit.',
            '#include "simulation/PeriodicContentLinks.h"',
            "",
            "#include <array>",
            "#include <cstdint>",
            "",
            "namespace",
            "{",
            f"constexpr std::array<PeriodicContentLink, {len(compiled_rows)}> records{{{{",
            *entries,
            "}};",
            "}",
            "",
            "std::span<PeriodicContentLink const> GetPeriodicContentLinks()",
            "{",
            "\treturn records;",
            "}",
            "",
        )
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("links", type=Path)
    parser.add_argument("related", type=Path, nargs="?")
    parser.add_argument("registry", type=Path, nargs="?")
    args = parser.parse_args(argv)
    args.output.write_text(
        render(args.links, args.related, args.registry),
        encoding="utf-8",
        newline="",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
