#!/usr/bin/env python3
"""Generate the compiled periodic-table UI model from the stable source map."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import sys
from typing import Sequence


PERIOD_RANGES = (
    (1, 2),
    (3, 10),
    (11, 18),
    (19, 36),
    (37, 54),
    (55, 86),
    (87, 118),
)

GROUPS = {
    1: 1, 2: 18,
    3: 1, 4: 2, 5: 13, 6: 14, 7: 15, 8: 16, 9: 17, 10: 18,
    11: 1, 12: 2, 13: 13, 14: 14, 15: 15, 16: 16, 17: 17, 18: 18,
}
for start in (19, 37):
    for offset in range(18):
        GROUPS[start + offset] = offset + 1
GROUPS.update({55: 1, 56: 2, 57: 3, 87: 1, 88: 2, 89: 3})
for atomic_number in range(72, 87):
    GROUPS[atomic_number] = atomic_number - 68
for atomic_number in range(104, 119):
    GROUPS[atomic_number] = atomic_number - 100
for atomic_number in (*range(58, 72), *range(90, 104)):
    GROUPS[atomic_number] = 0

GASES = {1, 2, 7, 8, 9, 10, 17, 18, 36, 54, 86}
LIQUIDS = {35, 80}
UNKNOWN_STATES = set(range(104, 119))
METALLOIDS = {5, 14, 32, 33, 51, 52, 85}
NONMETALS = {1, 2, 6, 7, 8, 9, 10, 15, 16, 17, 18, 34, 35, 36, 53, 54, 86}
RADIOACTIVE = {43, 61, *range(84, 119)}


def period_for(atomic_number: int) -> int:
    for period, (first, last) in enumerate(PERIOD_RANGES, 1):
        if first <= atomic_number <= last:
            return period
    raise ValueError(f"atomic number outside periodic table: {atomic_number}")


def family_for(atomic_number: int, group: int) -> str:
    if atomic_number == 1:
        return "hydrogen"
    if 58 <= atomic_number <= 71:
        return "lanthanide"
    if 90 <= atomic_number <= 103:
        return "actinide"
    return {
        1: "alkali_metal",
        2: "alkaline_earth_metal",
        13: "boron_group",
        14: "carbon_group",
        15: "pnictogen",
        16: "chalcogen",
        17: "halogen",
        18: "noble_gas",
    }.get(group, "transition_metal" if 3 <= group <= 12 else "unknown")


def series_for(atomic_number: int) -> str:
    if 58 <= atomic_number <= 71:
        return "lanthanide"
    if 90 <= atomic_number <= 103:
        return "actinide"
    if atomic_number >= 104:
        return "superheavy"
    return "main"


def state_for(atomic_number: int) -> str:
    if atomic_number in UNKNOWN_STATES:
        return "Unknown"
    if atomic_number in GASES:
        return "Gas"
    if atomic_number in LIQUIDS:
        return "Liquid"
    return "Solid"


def class_for(atomic_number: int) -> str:
    if atomic_number in METALLOIDS:
        return "Metalloid"
    if atomic_number in NONMETALS:
        return "Nonmetal"
    if atomic_number >= 113:
        return "Unknown"
    return "Metal"


def table_position(atomic_number: int, period: int, group: int) -> tuple[int, int]:
    if 58 <= atomic_number <= 71:
        return 7, atomic_number - 55
    if 90 <= atomic_number <= 103:
        return 8, atomic_number - 87
    return period - 1, group - 1


def cxx_string(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def read_source_map(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != 118:
        raise ValueError(f"source map has {len(rows)} rows instead of 118")
    return rows


def validate_output_path(path: Path) -> None:
    if path.suffix.lower() != ".cpp":
        raise ValueError(
            f"periodic table output must be a generated .cpp file, got: {path}"
        )


def render(source_map: Path) -> str:
    rows = read_source_map(source_map)
    output = [
        '#include "simulation/PeriodicTableData.h"',
        "",
        "#include <array>",
        "",
        "namespace",
        "{",
        "constexpr std::array<PeriodicElementRecord, 118> records{{",
    ]
    seen_ids: set[int] = set()
    for expected_atomic_number, row in enumerate(rows, 1):
        atomic_number = int(row["atomic_number"])
        if atomic_number != expected_atomic_number:
            raise ValueError(f"source map atomic order changed at {expected_atomic_number}")
        stable_id = int(row["stable_id"])
        if stable_id in seen_ids:
            raise ValueError(f"duplicate periodic stable ID {stable_id}")
        seen_ids.add(stable_id)
        identifier = row["official_mapping"] or row["omnipack_mapping"]
        implemented = row["status"] == "implemented"
        if implemented != bool(identifier):
            raise ValueError(
                f"atomic number {atomic_number} implementation and identifier disagree"
            )
        period = period_for(atomic_number)
        group = GROUPS[atomic_number]
        table_row, table_column = table_position(atomic_number, period, group)
        fields = (
            str(atomic_number), cxx_string(row["symbol"]), cxx_string(row["zh_name"]),
            cxx_string(row["en_name"]), cxx_string(identifier), str(stable_id),
            str(period), str(group), str(table_row), str(table_column),
            cxx_string(family_for(atomic_number, group)), cxx_string(series_for(atomic_number)),
            f"PeriodicStandardState::{state_for(atomic_number)}",
            f"PeriodicMaterialClass::{class_for(atomic_number)}",
            "true" if atomic_number in RADIOACTIVE else "false",
            "true" if implemented else "false",
        )
        output.append("\t{ " + ", ".join(fields) + " },")
    output.extend([
        "}};",
        "}",
        "",
        "std::span<PeriodicElementRecord const> GetPeriodicTableData()",
        "{",
        "\treturn records;",
        "}",
        "",
        "PeriodicElementRecord const *FindPeriodicElementByAtomicNumber(int atomicNumber)",
        "{",
        "\treturn atomicNumber >= 1 && atomicNumber <= int(records.size())",
        "\t\t? &records[atomicNumber - 1] : nullptr;",
        "}",
        "",
        "PeriodicElementRecord const *FindPeriodicElementByStableId(int stableId)",
        "{",
        "\tfor (auto const &record : records)",
        "\t\tif (record.stableId == stableId)",
        "\t\t\treturn &record;",
        "\treturn nullptr;",
        "}",
        "",
    ])
    return "\n".join(output)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("source_map", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    validate_output_path(args.output)
    rendered = render(args.source_map.resolve())
    if args.check:
        if not args.output.is_file() or args.output.read_text(encoding="utf-8") != rendered:
            print(f"ERROR {args.output}: generated periodic table data is stale", file=sys.stderr)
            return 1
        print("periodic-table-data: PASS rows=118")
        return 0
    args.output.write_text(rendered, encoding="utf-8", newline="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
