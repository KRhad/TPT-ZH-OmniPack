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
KIND_TOKENS = {
    "periodic_element": "PeriodicElement",
    "isotope": "Isotope",
    "inorganic_compound": "InorganicCompound",
}
GROUP_TOKENS = {
    "element": "Element", "isotope": "Isotope", "oxide": "Oxide",
    "hydroxide": "Hydroxide", "acid": "Acid", "base": "Base",
    "salt": "Salt", "halide": "Halide", "sulfide": "Sulfide",
    "nitride": "Nitride", "carbide": "Carbide", "hydride": "Hydride",
    "other": "Other",
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


def masks(atoms: list[int]) -> tuple[int, int]:
    low = 0
    high = 0
    for atom in atoms:
        if atom <= 64:
            low |= 1 << (atom - 1)
        else:
            high |= 1 << (atom - 65)
    return low, high


def render(path: Path) -> str:
    rows = read_rows(path)
    entries = []
    for row in rows:
        atoms = [int(item) for item in row["related_atomic_numbers"].split("|")]
        low, high = masks(atoms)
        entries.append(
            "\t{ "
            + ", ".join(
                (
                    cpp_string(row["tool_identifier"]),
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
            f"constexpr std::array<PeriodicContentLink, {len(rows)}> records{{{{",
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
    args = parser.parse_args(argv)
    args.output.write_text(render(args.links), encoding="utf-8", newline="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
