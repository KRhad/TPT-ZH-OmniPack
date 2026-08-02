#!/usr/bin/env python3
"""Generate the compiled, read-only element catalog from audited CSV sources."""

from __future__ import annotations

import csv
import pathlib
import re
import subprocess
import sys


ELEMENT_LIMIT = 1024
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
CONTENT_FIELDS = (
    "identifier",
    "recipe_en",
    "recipe_zh",
    "production_en",
    "production_zh",
    "use_en",
    "use_zh",
    "hazard_en",
    "hazard_zh",
)
OPTIONAL_CONTENT_PAIRS = (("production_en", "production_zh"),)
REQUIRED_CONTENT_FIELDS = tuple(
    field
    for field in CONTENT_FIELDS
    if all(field not in pair for pair in OPTIONAL_CONTENT_PAIRS)
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
CJK = re.compile(
    r"[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF"
    r"\U00020000-\U0002FA1F\U00030000-\U000323AF]"
)
ASCII_LETTER = re.compile(r"[A-Za-z]")


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


def read_content_records(
    content_path: pathlib.Path,
) -> tuple[dict[str, dict[str, str]], list[str]]:
    errors: list[str] = []
    try:
        with content_path.open("r", encoding="utf-8-sig", newline="") as content_file:
            reader = csv.DictReader(content_file)
            fieldnames = reader.fieldnames or []
            rows = list(reader)
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        return {}, [f"cannot read content registry: {exc}"]

    duplicate_columns = sorted(
        {field for field in fieldnames if field and fieldnames.count(field) > 1}
    )
    if any(not field for field in fieldnames):
        errors.append("content registry has an empty column name")
    if duplicate_columns:
        errors.append("content registry has duplicate columns: " + ", ".join(duplicate_columns))
    missing_columns = [field for field in CONTENT_FIELDS if field not in fieldnames]
    unexpected_columns = [field for field in fieldnames if field not in CONTENT_FIELDS]
    if missing_columns:
        errors.append("content registry is missing columns: " + ", ".join(missing_columns))
    if unexpected_columns:
        errors.append("content registry has unexpected columns: " + ", ".join(unexpected_columns))
    if errors:
        return {}, errors

    records: dict[str, dict[str, str]] = {}
    for line_number, row in enumerate(rows, start=2):
        if None in row or any(row.get(field) is None for field in fieldnames):
            errors.append(
                f"content registry:{line_number}: CSV row width does not match "
                f"the {len(fieldnames)}-column header"
            )
            continue
        clean: dict[str, str] = {}
        for field in CONTENT_FIELDS:
            value = row[field]
            assert isinstance(value, str)
            control = FORBIDDEN_CONTROL.search(value)
            if control:
                errors.append(
                    f"content registry:{line_number}: field {field!r} contains "
                    f"forbidden control character U+{ord(control.group()):04X}"
                )
            clean[field] = value.strip()
        if any(not clean[field] for field in REQUIRED_CONTENT_FIELDS):
            missing = next(field for field in REQUIRED_CONTENT_FIELDS if not clean[field])
            errors.append(f"content registry:{line_number}: empty required field {missing!r}")
            continue
        invalid_pair = next(
            (
                pair
                for pair in OPTIONAL_CONTENT_PAIRS
                if bool(clean[pair[0]]) != bool(clean[pair[1]])
            ),
            None,
        )
        if invalid_pair:
            errors.append(
                f"content registry:{line_number}: {invalid_pair[0]} and "
                f"{invalid_pair[1]} must both be empty or both be populated"
            )
            continue
        identifier = clean["identifier"]
        folded = identifier.casefold()
        if folded in records:
            errors.append(f"content registry:{line_number}: duplicate identifier {identifier}")
            continue
        for field in ("recipe_en", "production_en", "use_en", "hazard_en"):
            if clean[field] and not ASCII_LETTER.search(clean[field]):
                errors.append(
                    f"content registry:{line_number}: {field} must contain an ASCII letter"
                )
        for field in ("recipe_zh", "production_zh", "use_zh", "hazard_zh"):
            if clean[field] and not CJK.search(clean[field]):
                errors.append(
                    f"content registry:{line_number}: {field} must contain a CJK character"
                )
        records[folded] = clean
    return records, errors


def audit_content_registry(
    registry_path: pathlib.Path,
    content_path: pathlib.Path,
) -> tuple[dict[str, dict[str, str]], list[str]]:
    records, errors = read_content_records(content_path)
    try:
        with registry_path.open("r", encoding="utf-8-sig", newline="") as registry_file:
            registry_rows = list(csv.DictReader(registry_file))
    except (OSError, UnicodeDecodeError, csv.Error) as exc:
        return records, errors + [f"cannot read element registry: {exc}"]

    expected: dict[str, str] = {}
    for line_number, row in enumerate(registry_rows, start=2):
        identifier = (row.get("identifier") or "").strip()
        implementation = (row.get("implementation_status") or "").strip()
        if identifier.startswith("OMNI_PT_") and implementation == "implemented":
            expected[identifier.casefold()] = identifier

    for identifier, record in records.items():
        if identifier not in expected:
            errors.append(
                f"content registry:{record['identifier']}: has no implemented OmniPack element"
            )
        elif record["identifier"] != expected[identifier]:
            errors.append(
                f"content registry:{record['identifier']}: identifier must match "
                f"the canonical registry spelling {expected[identifier]}"
            )
    missing = sorted(identifier for identifier in expected if identifier not in records)
    if missing:
        errors.append(
            "content registry: missing implemented OmniPack elements: "
            + ", ".join(expected[identifier] for identifier in missing)
        )
    return records, errors


def main() -> int:
    if len(sys.argv) not in (3, 4, 5):
        print(
            "usage: generate_element_catalog.py OUTPUT_CPP ELEMENT_REGISTRY.csv "
            "[SOURCE_ROOT [ELEMENT_CONTENT.csv]]",
            file=sys.stderr,
        )
        return 2

    output_path = pathlib.Path(sys.argv[1])
    registry_path = pathlib.Path(sys.argv[2])
    content_records: dict[str, dict[str, str]] = {}
    if len(sys.argv) >= 4:
        source_root = pathlib.Path(sys.argv[3]).resolve()
        if not validate_repository(source_root, registry_path.resolve()):
            return 1
    if len(sys.argv) == 5:
        content_path = pathlib.Path(sys.argv[4])
        content_records, content_errors = audit_content_registry(
            registry_path, content_path
        )
        if content_errors:
            for error in content_errors:
                print(f"element catalog: {error}", file=sys.stderr)
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
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("recipe_en", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("recipe_zh", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("production_en", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("production_zh", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("use_en", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("use_zh", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("hazard_en", "")),
            cpp_string(content_records.get(row["identifier"].casefold(), {}).get("hazard_zh", "")),
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
