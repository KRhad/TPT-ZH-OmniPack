#!/usr/bin/env python3
"""Generate the compiled, read-only element catalog from ELEMENT_REGISTRY.csv."""

from __future__ import annotations

import csv
import pathlib
import re
import subprocess
import sys


ELEMENT_LIMIT = 512
FIELDS = (
    "identifier",
    "display_code",
    "english_name",
    "chinese_name",
    "source_mod",
    "source_id",
    "stable_id",
    "menu_category",
    "element_state",
    "default_enabled",
    "duplicate_of",
    "save_compatibility",
    "implementation_status",
    "test_status",
    "source_commit",
    "english_description",
    "chinese_description",
    "license",
    "notes",
)
REQUIRED_FIELDS = (
    "identifier",
    "display_code",
    "english_name",
    "chinese_name",
    "source_mod",
    "source_id",
    "menu_category",
    "element_state",
    "default_enabled",
    "save_compatibility",
    "implementation_status",
    "test_status",
    "source_commit",
    "english_description",
    "chinese_description",
    "license",
)
CANONICAL_DECIMAL = re.compile(r"0|[1-9][0-9]*")
FORBIDDEN_CONTROL = re.compile(r"[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]")


def cpp_string(value: str) -> str:
    return (
        '"'
        + value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\r", "\\r")
        .replace("\n", "\\n")
        .replace("\t", "\\t")
        + '"'
    )


def validate_repository(source_root: pathlib.Path, registry_path: pathlib.Path) -> bool:
    checker = pathlib.Path(__file__).with_name("element_registry_check.py")
    completed = subprocess.run(
        [
            sys.executable,
            str(checker),
            "--root",
            str(source_root),
            "--registry",
            str(registry_path),
            "--quiet",
        ],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if completed.returncode:
        print(
            "element catalog: repository registry validation failed",
            file=sys.stderr,
        )
        if completed.stdout:
            print(completed.stdout.rstrip(), file=sys.stderr)
        if completed.stderr:
            print(completed.stderr.rstrip(), file=sys.stderr)
        return False
    return True


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print(
            "usage: generate_element_catalog.py OUTPUT_CPP ELEMENT_REGISTRY.csv "
            "[SOURCE_ROOT]",
            file=sys.stderr,
        )
        return 2

    output_path = pathlib.Path(sys.argv[1])
    registry_path = pathlib.Path(sys.argv[2])
    if len(sys.argv) == 4:
        source_root = pathlib.Path(sys.argv[3]).resolve()
        if not validate_repository(source_root, registry_path.resolve()):
            return 1

    with registry_path.open("r", encoding="utf-8-sig", newline="") as registry_file:
        reader = csv.DictReader(registry_file)
        fieldnames = reader.fieldnames or []
        duplicate_columns = sorted(
            {
                field
                for field in fieldnames
                if field and fieldnames.count(field) > 1
            }
        )
        if any(not field for field in fieldnames):
            print("element catalog: empty column name", file=sys.stderr)
            return 1
        if duplicate_columns:
            print(
                "element catalog: duplicate columns: "
                + ", ".join(duplicate_columns),
                file=sys.stderr,
            )
            return 1
        missing_columns = [
            field for field in FIELDS if field not in fieldnames
        ]
        if missing_columns:
            print(
                "element catalog: missing columns: " + ", ".join(missing_columns),
                file=sys.stderr,
            )
            return 1
        rows = list(reader)

    identifiers: set[str] = set()
    stable_ids: set[int] = set()
    parsed_rows: list[tuple[int, dict[str, str]]] = []
    for line_number, row in enumerate(rows, start=2):
        if None in row or any(row.get(field) is None for field in fieldnames):
            print(
                f"element catalog:{line_number}: CSV row width does not match "
                f"the {len(fieldnames)}-column header",
                file=sys.stderr,
            )
            return 1
        clean_row: dict[str, str] = {}
        for field in fieldnames:
            value = row[field]
            assert isinstance(value, str)
            control = FORBIDDEN_CONTROL.search(value)
            if control:
                print(
                    f"element catalog:{line_number}: field {field!r} contains "
                    f"forbidden control character U+{ord(control.group()):04X}",
                    file=sys.stderr,
                )
                return 1
            if field in FIELDS:
                clean_row[field] = value.strip()

        for field in REQUIRED_FIELDS:
            if not clean_row[field]:
                print(
                    f"element catalog:{line_number}: empty required field {field!r}",
                    file=sys.stderr,
                )
                return 1

        identifier = clean_row["identifier"]
        stable_id_text = row.get("stable_id")
        if (
            stable_id_text is None
            or CANONICAL_DECIMAL.fullmatch(stable_id_text) is None
        ):
            print(
                f"element catalog:{line_number}: invalid stable_id "
                f"{stable_id_text!r}; expected a canonical decimal integer",
                file=sys.stderr,
            )
            return 1
        try:
            stable_id = int(stable_id_text)
        except ValueError:
            print(
                f"element catalog:{line_number}: invalid stable_id "
                f"{stable_id_text!r}; expected a canonical decimal integer",
                file=sys.stderr,
            )
            return 1
        if not 0 <= stable_id < ELEMENT_LIMIT:
            print(
                f"element catalog:{line_number}: stable_id {stable_id} is outside "
                f"the supported range 0..{ELEMENT_LIMIT - 1}",
                file=sys.stderr,
            )
            return 1
        folded_identifier = identifier.casefold()
        if folded_identifier in identifiers:
            print(
                f"element catalog:{line_number}: duplicate identifier {identifier}",
                file=sys.stderr,
            )
            return 1
        if stable_id in stable_ids:
            print(
                f"element catalog:{line_number}: duplicate stable_id {stable_id}",
                file=sys.stderr,
            )
            return 1
        identifiers.add(folded_identifier)
        stable_ids.add(stable_id)
        parsed_rows.append((stable_id, clean_row))

    parsed_rows.sort(key=lambda item: item[0])
    entries: list[str] = []
    for stable_id, row in parsed_rows:
        values = [
            cpp_string(row["identifier"]),
            cpp_string(row["display_code"]),
            cpp_string(row["english_name"]),
            cpp_string(row["chinese_name"]),
            cpp_string(row["source_mod"]),
            cpp_string(row["source_id"]),
            str(stable_id),
            cpp_string(row["menu_category"]),
            cpp_string(row["element_state"]),
            cpp_string(row["default_enabled"]),
            cpp_string(row["duplicate_of"]),
            cpp_string(row["save_compatibility"]),
            cpp_string(row["implementation_status"]),
            cpp_string(row["test_status"]),
            cpp_string(row["source_commit"]),
            cpp_string(row["english_description"]),
            cpp_string(row["chinese_description"]),
            cpp_string(row["license"]),
            cpp_string(row["notes"]),
        ]
        entries.append("\t{ " + ", ".join(values) + " },")

    output = f"""// Generated from docs/ELEMENT_REGISTRY.csv. Do not edit this build artifact.
#include "gui/elementsearch/ElementCatalog.h"

#include <array>

namespace
{{
constexpr std::array<ElementCatalogRecord, {len(entries)}> catalog{{ {{
{chr(10).join(entries)}
}} }};
}}

std::span<ElementCatalogRecord const> GetElementCatalog()
{{
\treturn catalog;
}}

ElementCatalogRecord const *FindElementCatalogByIdentifier(std::string_view identifier)
{{
\tfor (auto const &record : catalog)
\t{{
\t\tif (record.identifier == identifier)
\t\t{{
\t\t\treturn &record;
\t\t}}
\t}}
\treturn nullptr;
}}

ElementCatalogRecord const *FindElementCatalogByStableId(int stableId)
{{
\tfor (auto const &record : catalog)
\t{{
\t\tif (record.stableId == stableId)
\t\t{{
\t\t\treturn &record;
\t\t}}
\t}}
\treturn nullptr;
}}
"""
    with output_path.open("w", encoding="utf-8", newline="") as output_file:
        output_file.write(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
