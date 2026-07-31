#!/usr/bin/env python3
"""Read-only integrity gate for TPT-ZH-OmniPack element registration.

The authoritative official slot lock is pinned to The Powder Toy 100.0.399
commit bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd.  This checker deliberately
does not offer an update/write mode: changing the official lock must be an
explicit, reviewed source migration.

The default invocation is suitable for Meson/CI:

    python tools/element_registry_check.py

It reads only repository files and exits 0 on success, 1 on validation
failure, and 2 for command-line or I/O errors.
"""

from __future__ import annotations

import argparse
import ast
import csv
import hashlib
import io
import json
import math
import operator
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Mapping, Sequence


OFFICIAL_COMMIT = "bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd"
OFFICIAL_VERSION = "100.0.399"
OFFICIAL_REPOSITORY = "The-Powder-Toy/The-Powder-Toy"
OFFICIAL_SLOT_FIRST = 0
OFFICIAL_SLOT_LAST = 195
OFFICIAL_RESERVED_SLOT = 146
OFFICIAL_RESERVED_IDENTIFIER = "RESERVED_PT_146"
OFFICIAL_RESERVED_DISPLAY_CODE = "----"
OMNI_RESERVED_REPOSITORY = "TPT-ZH-OmniPack/reserved"
OMNI_RESERVED_COMMIT = "f28cdcb734c6829ae2f69ca10245d494704a8164"
OMNI_RESERVED_FIRST = 196
OMNI_RESERVED_LAST = 255

# SHA-256 over the lock header and rows, using US (0x1f) between fields and LF
# between records.  Filled after generating the audited bff38ce6 lock table.
OFFICIAL_LOCK_SHA256 = "63c27aa379c39ec062a8172e185a147b5b30716310b5edd49fd1e5b11a3f55e3"

LOCK_COLUMNS = (
    "official_commit",
    "tpt_version",
    "stable_id",
    "slot_status",
    "meson_name",
    "identifier",
    "display_code",
    "source_file",
)

# The first fourteen fields preserve the registry schema requested at project
# inception.  Additional fields make source and description checks auditable.
REGISTRY_COLUMNS = (
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
    "is_duplicate",
    "meson_name",
    "source_file",
    "source_commit",
    "english_description",
    "chinese_description",
    "license",
    "notes",
    "code",
    "zh_name",
    "en_name",
    "module",
    "source",
    "production",
    "uses",
    "hazards",
    "controls",
    "cleanup",
    "status",
    "tests",
)

UPSTREAM_GAMEPLAY_SENTINEL = "upstream_managed_by_official_tpt"
RESERVED_GAMEPLAY_SENTINEL = "reserved_slot_no_element"
OMNIPACK_MODULE_RANGES = (
    (256, 287, "metallurgy"),
    (288, 327, "biology"),
    (328, 359, "nuclear"),
    (360, 511, "chemistry"),
)

SLOT_STATUSES = frozenset({"active", "reserved"})
ELEMENT_STATES = frozenset(
    {"none", "powder", "liquid", "solid", "gas", "energy", "special", "reserved"}
)
SAVE_COMPATIBILITY = frozenset(
    {
        "official-locked",
        "compatible",
        "migratable",
        "partial",
        "incompatible",
        "reserved-slot",
        "unknown",
    }
)
IMPLEMENTATION_STATUSES = frozenset(
    {"implemented", "partial", "planned", "disabled", "reserved", "removed"}
)
TEST_STATUSES = frozenset(
    {
        "untested",
        "source-verified",
        "unit-tested",
        "runtime-tested",
        "stress-tested",
        "lock-verified",
        "blocked",
    }
)
LICENSES = frozenset(
    {
        "GPL-3.0-only",
        "GPL-3.0-or-later",
        "GPL-2.0-only",
        "GPL-2.0-or-later",
    }
)

ASCII_IDENTIFIER = re.compile(r"^[A-Z][A-Z0-9_]*$")
ASCII_MESON_NAME = re.compile(r"^[A-Z][A-Z0-9_]*$")
ASCII_DISPLAY_CODE = re.compile(r"^[\x21-\x7E]+$")
HEX_COMMIT = re.compile(r"^[0-9a-f]{40}$")
CJK = re.compile(r"[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]")
ASCII_LETTER = re.compile(r"[A-Za-z]")
FORBIDDEN_CONTROL = re.compile(r"[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]")

NUMERIC_ASSIGNMENTS = {
    "Advection": (-10.0, 10.0),
    "AirDrag": (-10.0, 10.0),
    "AirLoss": (-10.0, 10.0),
    "Loss": (-10.0, 10.0),
    "Collision": (-10.0, 10.0),
    "Gravity": (-10.0, 10.0),
    "Diffusion": (-10.0, 10.0),
    "HotAir": (-1000.0, 1000.0),
    "Flammable": (0.0, 10000.0),
    "Explosive": (0.0, 10000.0),
    "Meltable": (0.0, 10000.0),
    "Hardness": (0.0, 10000.0),
    "Weight": (-10000.0, 10000.0),
    "HeatConduct": (0.0, 255.0),
    "HeatCapacity": (sys.float_info.min, 1_000_000.0),
    "LowPressure": (-257.0, 257.0),
    "HighPressure": (-257.0, 257.0),
    "LowTemperature": (-1.0, 10000.0),
    "HighTemperature": (-1.0, 10000.0),
    "MenuVisible": (0.0, 1.0),
    "Enabled": (0.0, 1.0),
}

NUMERIC_CONSTANTS = {
    "CFDS": 1.0,
    "R_TEMP": 273.15,
    "MIN_TEMP": 0.0,
    "MAX_TEMP": 9999.0,
    "MIN_PRESSURE": -256.0,
    "MAX_PRESSURE": 256.0,
    "IPL": -257.0,
    "IPH": 257.0,
    "ITL": -1.0,
    "ITH": 10000.0,
}

STATE_FROM_PROPERTY = (
    ("TYPE_PART", "powder"),
    ("TYPE_LIQUID", "liquid"),
    ("TYPE_SOLID", "solid"),
    ("TYPE_GAS", "gas"),
    ("TYPE_ENERGY", "energy"),
)


@dataclass(frozen=True)
class Finding:
    code: str
    path: str
    message: str
    row: int | None = None

    def text(self) -> str:
        location = self.path
        if self.row is not None:
            location += f":{self.row}"
        return f"{location}: [{self.code}] {self.message}"


class Findings:
    def __init__(self, root: Path):
        self.root = root
        self.errors: list[Finding] = []

    def relative(self, path: Path) -> str:
        try:
            return path.resolve().relative_to(self.root.resolve()).as_posix()
        except (OSError, ValueError):
            return path.as_posix()

    def add(
        self,
        code: str,
        path: Path | str,
        message: str,
        row: int | None = None,
    ) -> None:
        shown = self.relative(path) if isinstance(path, Path) else path
        self.errors.append(Finding(code, shown, message, row))


@dataclass(frozen=True)
class ElementSource:
    stable_id: int
    meson_name: str
    source_file: str
    identifier: str
    display_code: str
    menu_category: str
    enabled: bool
    element_state: str
    description_key: str


def read_text(path: Path, findings: Findings) -> str | None:
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        findings.add("UTF8", path, f"not valid UTF-8: {exc}")
    except OSError as exc:
        findings.add("IO", path, str(exc))
    return None


def parse_csv_text(
    text: str,
    path: Path,
    expected_columns: Sequence[str],
    findings: Findings,
) -> list[dict[str, str]]:
    if text.startswith("\ufeff"):
        text = text[1:]
    try:
        records = list(csv.reader(io.StringIO(text, newline="")))
    except csv.Error as exc:
        findings.add("CSV_PARSE", path, str(exc))
        return []
    if not records:
        findings.add("CSV_EMPTY", path, "file has no header")
        return []

    header = records[0]
    if len(header) != len(set(header)):
        findings.add("CSV_HEADER_DUPLICATE", path, "header contains duplicate names", 1)
    if tuple(header) != tuple(expected_columns):
        findings.add(
            "CSV_SCHEMA",
            path,
            "expected columns in this order: " + ",".join(expected_columns),
            1,
        )

    rows: list[dict[str, str]] = []
    for row_number, values in enumerate(records[1:], start=2):
        if not values or all(value == "" for value in values):
            findings.add("CSV_BLANK_ROW", path, "blank rows are not allowed", row_number)
            continue
        if len(values) != len(header):
            findings.add(
                "CSV_WIDTH",
                path,
                f"expected {len(header)} fields, found {len(values)}",
                row_number,
            )
            continue
        row = dict(zip(header, values))
        for key, value in row.items():
            if value != value.strip():
                findings.add(
                    "CSV_WHITESPACE",
                    path,
                    f"{key!r} has leading or trailing whitespace",
                    row_number,
                )
            if FORBIDDEN_CONTROL.search(value):
                findings.add(
                    "CSV_CONTROL",
                    path,
                    f"{key!r} contains a forbidden control character",
                    row_number,
                )
        row["_row_number"] = str(row_number)
        rows.append(row)
    return rows


def read_csv_file(
    path: Path,
    expected_columns: Sequence[str],
    findings: Findings,
) -> list[dict[str, str]]:
    text = read_text(path, findings)
    if text is None:
        return []
    return parse_csv_text(text, path, expected_columns, findings)


def strict_int(
    value: str,
    field: str,
    path: Path,
    row_number: int,
    findings: Findings,
) -> int | None:
    if not re.fullmatch(r"0|[1-9][0-9]*", value):
        findings.add("INTEGER", path, f"{field!r} must be a canonical non-negative integer", row_number)
        return None
    return int(value)


def strict_bool(
    value: str,
    field: str,
    path: Path,
    row_number: int,
    findings: Findings,
) -> bool | None:
    if value not in {"true", "false"}:
        findings.add("BOOLEAN", path, f"{field!r} must be 'true' or 'false'", row_number)
        return None
    return value == "true"


def reserved_registry_expectations(stable_id: int) -> dict[str, str]:
    if stable_id == OFFICIAL_RESERVED_SLOT:
        return {
            "identifier": OFFICIAL_RESERVED_IDENTIFIER,
            "display_code": OFFICIAL_RESERVED_DISPLAY_CODE,
            "english_name": "Reserved Slot 146",
            "chinese_name": "保留槽位 146",
            "source_mod": OFFICIAL_REPOSITORY,
            "source_id": str(stable_id),
            "source_commit": OFFICIAL_COMMIT,
            "english_description": (
                "Reserved official element slot; never reuse this numeric ID."
            ),
            "chinese_description": (
                "官方保留元素槽位；不得重新使用此数字 ID。"
            ),
            "license": "GPL-3.0-only",
            "meson_name": "disabler()",
            "source_file": "",
            "menu_category": "RESERVED",
            "element_state": "reserved",
            "default_enabled": "false",
            "is_duplicate": "false",
            "duplicate_of": "",
            "save_compatibility": "reserved-slot",
            "implementation_status": "reserved",
            "test_status": "lock-verified",
        }
    return {
        "identifier": f"RESERVED_PT_{stable_id}",
        "display_code": "----",
        "english_name": f"Reserved Slot {stable_id}",
        "chinese_name": f"保留槽位 {stable_id}",
        "source_mod": OMNI_RESERVED_REPOSITORY,
        "source_id": str(stable_id),
        "source_commit": OMNI_RESERVED_COMMIT,
        "english_description": (
            f"Stable ID {stable_id} is reserved and has no selectable element."
        ),
        "chinese_description": (
            f"稳定 ID {stable_id} 为保留槽位，当前没有可选择元素。"
        ),
        "license": "GPL-3.0-only",
        "meson_name": "disabler()",
        "source_file": "",
        "menu_category": "RESERVED",
        "element_state": "reserved",
        "default_enabled": "false",
        "is_duplicate": "false",
        "duplicate_of": "",
        "save_compatibility": "reserved-slot",
        "implementation_status": "reserved",
        "test_status": "lock-verified",
    }


def canonical_lock_digest(rows: Sequence[Mapping[str, str]]) -> str:
    lines = ["\x1f".join(LOCK_COLUMNS)]
    for row in sorted(rows, key=lambda item: int(item["stable_id"])):
        lines.append("\x1f".join(row[column] for column in LOCK_COLUMNS))
    return hashlib.sha256(("\n".join(lines) + "\n").encode("utf-8")).hexdigest()


def parse_meson_slots(path: Path, findings: Findings) -> list[str | None]:
    text = read_text(path, findings)
    if text is None:
        return []
    match = re.search(r"simulation_elem_names\s*=\s*\[(.*?)^\]", text, re.MULTILINE | re.DOTALL)
    if not match:
        findings.add("MESON_LIST", path, "simulation_elem_names list not found")
        return []

    slots: list[str | None] = []
    for offset, raw_line in enumerate(match.group(1).splitlines(), start=1):
        line = raw_line.split("#", 1)[0].strip().rstrip(",").strip()
        if not line:
            continue
        if line == "disabler()":
            slots.append(None)
            continue
        name_match = re.fullmatch(r"'([A-Z][A-Z0-9_]*)'", line)
        if not name_match:
            findings.add(
                "MESON_ENTRY",
                path,
                f"unsupported element list entry {line!r}",
                text[: match.start(1)].count("\n") + offset + 1,
            )
            continue
        slots.append(name_match.group(1))
    return slots


def parse_pt_num(path: Path, findings: Findings) -> int | None:
    text = read_text(path, findings)
    if text is None:
        return None
    bits_match = re.search(r"constexpr\s+int\s+PMAPBITS\s*=\s*([0-9]+)\s*;", text)
    num_match = re.search(
        r"constexpr\s+int\s+PT_NUM\s*=\s*1\s*<<\s*PMAPBITS\s*;",
        text,
    )
    if not bits_match or not num_match:
        findings.add("PT_NUM", path, "expected PMAPBITS and 'PT_NUM = 1 << PMAPBITS'")
        return None
    bits = int(bits_match.group(1))
    if not 1 <= bits <= 30:
        findings.add("PT_NUM", path, f"PMAPBITS={bits} is outside the supported check range")
        return None
    return 1 << bits


def parse_menu_categories(path: Path, findings: Findings) -> set[str]:
    text = read_text(path, findings)
    if text is None:
        return set()
    categories = set(re.findall(r"constexpr\s+int\s+(SC_[A-Z_]+)\s*=", text))
    if not categories:
        findings.add("MENU_SCHEMA", path, "no SC_* menu categories found")
    return categories


def source_line(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def one_assignment(
    text: str,
    pattern: str,
    field: str,
    path: Path,
    findings: Findings,
) -> str:
    matches = list(re.finditer(pattern, text, re.MULTILINE))
    if len(matches) != 1:
        findings.add(
            "SOURCE_ASSIGNMENT",
            path,
            f"expected exactly one constructor assignment for {field}, found {len(matches)}",
        )
        return ""
    return matches[0].group(1)


def infer_state(properties: str, display_code: str) -> str:
    for marker, state in STATE_FROM_PROPERTY:
        if re.search(rf"\b{marker}\b", properties):
            return state
    if display_code == "NONE":
        return "none"
    return "special"


_BIN_OPS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.Div: operator.truediv,
}
_UNARY_OPS = {ast.UAdd: operator.pos, ast.USub: operator.neg}


def eval_numeric_expression(expression: str) -> float | None:
    expression = expression.split("//", 1)[0].strip()
    expression = re.sub(
        r"(?<![A-Za-z0-9_])((?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)[fF]\b",
        r"\1",
        expression,
    )
    try:
        tree = ast.parse(expression, mode="eval")
    except SyntaxError:
        return None

    def visit(node: ast.AST) -> float:
        if isinstance(node, ast.Expression):
            return visit(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return float(node.value)
        if isinstance(node, ast.Name) and node.id in NUMERIC_CONSTANTS:
            return NUMERIC_CONSTANTS[node.id]
        if isinstance(node, ast.BinOp) and type(node.op) in _BIN_OPS:
            return _BIN_OPS[type(node.op)](visit(node.left), visit(node.right))
        if isinstance(node, ast.UnaryOp) and type(node.op) in _UNARY_OPS:
            return _UNARY_OPS[type(node.op)](visit(node.operand))
        raise ValueError

    try:
        return float(visit(tree))
    except (ArithmeticError, ValueError):
        return None


def validate_numeric_assignments(text: str, path: Path, findings: Findings) -> None:
    nonfinite = re.search(
        r"\b(?:NAN|INFINITY|HUGE_VALF?|numeric_limits\s*<[^>]+>\s*::\s*(?:infinity|quiet_NaN))\b",
        text,
        re.IGNORECASE,
    )
    if nonfinite:
        findings.add(
            "NONFINITE",
            path,
            "source contains a non-finite numeric expression",
            source_line(text, nonfinite.start()),
        )

    number_pattern = re.compile(
        r"(?<![A-Za-z0-9_])(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?[fF]?"
    )
    for match in number_pattern.finditer(text):
        token = match.group(0).rstrip("fF")
        try:
            value = float(token)
        except ValueError:
            continue
        if not math.isfinite(value):
            findings.add(
                "NONFINITE_LITERAL",
                path,
                f"numeric literal {match.group(0)!r} is not finite",
                source_line(text, match.start()),
            )

    assignment_pattern = re.compile(
        r"^\s*(" + "|".join(map(re.escape, NUMERIC_ASSIGNMENTS)) + r")\s*=\s*([^;]+);",
        re.MULTILINE,
    )
    for match in assignment_pattern.finditer(text):
        field, expression = match.groups()
        value = eval_numeric_expression(expression)
        if value is None:
            continue
        low, high = NUMERIC_ASSIGNMENTS[field]
        if not math.isfinite(value):
            findings.add(
                "NONFINITE",
                path,
                f"{field} evaluates to a non-finite value",
                source_line(text, match.start()),
            )
        elif not low <= value <= high:
            findings.add(
                "NUMERIC_RANGE",
                path,
                f"{field}={value:g} is outside the obvious legal range [{low:g}, {high:g}]",
                source_line(text, match.start()),
            )


def parse_element_source(
    root: Path,
    stable_id: int,
    meson_name: str,
    findings: Findings,
) -> ElementSource | None:
    relative = f"src/simulation/elements/{meson_name}.cpp"
    path = root / relative
    text = read_text(path, findings)
    if text is None:
        return None

    identifier = one_assignment(
        text,
        r'^\s*Identifier\s*=\s*"([^"]+)"\s*;',
        "Identifier",
        path,
        findings,
    )
    display_code = one_assignment(
        text,
        r'^\s*Name\s*=\s*"([^"]+)"\s*;',
        "Name",
        path,
        findings,
    )
    menu_category = one_assignment(
        text,
        r"^\s*MenuSection\s*=\s*(SC_[A-Z_]+)\s*;",
        "MenuSection",
        path,
        findings,
    )
    enabled_text = one_assignment(
        text,
        r"^\s*Enabled\s*=\s*([01])\s*;",
        "Enabled",
        path,
        findings,
    )
    menu_visible = one_assignment(
        text,
        r"^\s*MenuVisible\s*=\s*([01])\s*;",
        "MenuVisible",
        path,
        findings,
    )
    properties = one_assignment(
        text,
        r"^\s*Properties\s*=\s*([^;]+)\s*;",
        "Properties",
        path,
        findings,
    )
    description_key = one_assignment(
        text,
        r'^\s*Description\s*=\s*Localization::Ref\(\)\.Tr\("([^"]+)"\)\s*;',
        "localized Description",
        path,
        findings,
    )

    if menu_visible not in {"0", "1"}:
        findings.add("SOURCE_BOOLEAN", path, "MenuVisible must be 0 or 1")
    if identifier and not ASCII_IDENTIFIER.fullmatch(identifier):
        findings.add("SOURCE_IDENTIFIER", path, f"Identifier {identifier!r} is not uppercase ASCII")
    if display_code and not ASCII_DISPLAY_CODE.fullmatch(display_code):
        findings.add("SOURCE_DISPLAY", path, f"Name {display_code!r} is not printable ASCII")
    expected_key = f"sim.elem.{identifier}" if identifier else ""
    if description_key and description_key != expected_key:
        findings.add(
            "SOURCE_DESCRIPTION_KEY",
            path,
            f"description key {description_key!r} does not match {expected_key!r}",
        )

    validate_numeric_assignments(text, path, findings)
    if not all((identifier, display_code, menu_category, enabled_text, properties, description_key)):
        return None
    return ElementSource(
        stable_id=stable_id,
        meson_name=meson_name,
        source_file=relative,
        identifier=identifier,
        display_code=display_code,
        menu_category=menu_category,
        enabled=enabled_text == "1",
        element_state=infer_state(properties, display_code),
        description_key=description_key,
    )


def validate_lock(
    rows: Sequence[dict[str, str]],
    path: Path,
    findings: Findings,
) -> dict[int, dict[str, str]]:
    by_id: dict[int, dict[str, str]] = {}
    identifiers: dict[str, int] = {}
    meson_names: dict[str, int] = {}
    for row in rows:
        row_number = int(row["_row_number"])
        stable_id = strict_int(row.get("stable_id", ""), "stable_id", path, row_number, findings)
        if stable_id is None:
            continue
        if stable_id in by_id:
            findings.add("LOCK_DUPLICATE_ID", path, f"stable_id {stable_id} is duplicated", row_number)
        by_id[stable_id] = row

        if row.get("official_commit") != OFFICIAL_COMMIT:
            findings.add("LOCK_COMMIT", path, f"official_commit must be {OFFICIAL_COMMIT}", row_number)
        if row.get("tpt_version") != OFFICIAL_VERSION:
            findings.add("LOCK_VERSION", path, f"tpt_version must be {OFFICIAL_VERSION}", row_number)
        if row.get("slot_status") not in SLOT_STATUSES:
            findings.add("LOCK_STATUS", path, f"invalid slot_status {row.get('slot_status')!r}", row_number)

        if stable_id == OFFICIAL_RESERVED_SLOT:
            if row.get("slot_status") != "reserved":
                findings.add("LOCK_RESERVED", path, "slot 146 must be reserved", row_number)
            if row.get("meson_name") != "disabler()":
                findings.add("LOCK_RESERVED", path, "reserved slot must record meson_name disabler()", row_number)
            if row.get("identifier") != OFFICIAL_RESERVED_IDENTIFIER:
                findings.add(
                    "LOCK_RESERVED",
                    path,
                    f"reserved slot identifier must be {OFFICIAL_RESERVED_IDENTIFIER}",
                    row_number,
                )
            if row.get("display_code") != OFFICIAL_RESERVED_DISPLAY_CODE:
                findings.add(
                    "LOCK_RESERVED",
                    path,
                    f"reserved slot display_code must be {OFFICIAL_RESERVED_DISPLAY_CODE}",
                    row_number,
                )
            if row.get("source_file"):
                findings.add("LOCK_RESERVED", path, "reserved slot source_file must be empty", row_number)
            continue

        if row.get("slot_status") != "active":
            findings.add("LOCK_ACTIVE", path, "official non-reserved slots must be active", row_number)
        identifier = row.get("identifier", "")
        meson_name = row.get("meson_name", "")
        display_code = row.get("display_code", "")
        expected_source = f"src/simulation/elements/{meson_name}.cpp"
        if not ASCII_IDENTIFIER.fullmatch(identifier):
            findings.add("LOCK_IDENTIFIER", path, f"invalid ASCII identifier {identifier!r}", row_number)
        if not ASCII_MESON_NAME.fullmatch(meson_name):
            findings.add("LOCK_MESON", path, f"invalid meson_name {meson_name!r}", row_number)
        if not display_code or not ASCII_DISPLAY_CODE.fullmatch(display_code):
            findings.add("LOCK_DISPLAY", path, f"invalid display_code {display_code!r}", row_number)
        if row.get("source_file") != expected_source:
            findings.add("LOCK_SOURCE", path, f"source_file must be {expected_source}", row_number)

        folded_identifier = identifier.casefold()
        if folded_identifier in identifiers:
            findings.add(
                "LOCK_IDENTIFIER_CASE",
                path,
                f"identifier conflicts case-insensitively with slot {identifiers[folded_identifier]}",
                row_number,
            )
        identifiers[folded_identifier] = stable_id
        folded_meson = meson_name.casefold()
        if folded_meson in meson_names:
            findings.add(
                "LOCK_MESON_CASE",
                path,
                f"meson_name conflicts case-insensitively with slot {meson_names[folded_meson]}",
                row_number,
            )
        meson_names[folded_meson] = stable_id

    expected_ids = set(range(OFFICIAL_SLOT_FIRST, OFFICIAL_SLOT_LAST + 1))
    actual_ids = set(by_id)
    missing = sorted(expected_ids - actual_ids)
    extra = sorted(actual_ids - expected_ids)
    if missing:
        findings.add("LOCK_IDS", path, f"missing official stable IDs: {missing}")
    if extra:
        findings.add("LOCK_IDS", path, f"unexpected official stable IDs: {extra}")

    if rows and OFFICIAL_LOCK_SHA256 != "PENDING":
        digest = canonical_lock_digest(rows)
        if digest != OFFICIAL_LOCK_SHA256:
            findings.add(
                "LOCK_DRIFT",
                path,
                f"canonical SHA-256 {digest} does not match pinned {OFFICIAL_LOCK_SHA256}",
            )
    elif rows:
        findings.add("LOCK_DIGEST_UNSET", path, "checker has no pinned official lock digest")
    return by_id


def load_language(path: Path, findings: Findings) -> dict[str, str]:
    text = read_text(path, findings)
    if text is None:
        return {}
    try:
        value = json.loads(text)
    except json.JSONDecodeError as exc:
        findings.add("LANG_JSON", path, str(exc), exc.lineno)
        return {}
    if not isinstance(value, dict):
        findings.add("LANG_JSON", path, "top-level value must be an object")
        return {}
    return {str(key): item if isinstance(item, str) else "" for key, item in value.items()}


def validate_registry(
    rows: Sequence[dict[str, str]],
    path: Path,
    root: Path,
    lock_by_id: Mapping[int, Mapping[str, str]],
    sources_by_id: Mapping[int, ElementSource],
    slots: Sequence[str | None],
    pt_num: int | None,
    menu_categories: set[str],
    english: Mapping[str, str],
    chinese: Mapping[str, str],
    findings: Findings,
) -> dict[int, dict[str, str]]:
    by_id: dict[int, dict[str, str]] = {}
    identifier_owner: dict[str, int] = {}
    display_owner: dict[str, int] = {}

    for row in rows:
        row_number = int(row["_row_number"])
        stable_id = strict_int(row.get("stable_id", ""), "stable_id", path, row_number, findings)
        source_id = strict_int(row.get("source_id", ""), "source_id", path, row_number, findings)
        default_enabled = strict_bool(
            row.get("default_enabled", ""),
            "default_enabled",
            path,
            row_number,
            findings,
        )
        is_duplicate = strict_bool(
            row.get("is_duplicate", ""),
            "is_duplicate",
            path,
            row_number,
            findings,
        )
        if stable_id is None:
            continue

        alias_fields = {
            "code": "display_code",
            "zh_name": "chinese_name",
            "en_name": "english_name",
            "source": "source_mod",
            "status": "implementation_status",
            "tests": "test_status",
        }
        for alias, canonical in alias_fields.items():
            if row.get(alias) != row.get(canonical):
                findings.add(
                    "REGISTRY_ALIAS",
                    path,
                    f"{alias} must exactly match {canonical}",
                    row_number,
                )

        slot_is_reserved = stable_id < len(slots) and slots[stable_id] is None
        if stable_id <= OFFICIAL_SLOT_LAST:
            expected_module = "official_reserved" if slot_is_reserved else "official"
        elif slot_is_reserved:
            expected_module = "omnipack_reserved"
        else:
            expected_module = next(
                (
                    module
                    for first, last, module in OMNIPACK_MODULE_RANGES
                    if first <= stable_id <= last
                ),
                "",
            )
        if row.get("module") != expected_module:
            findings.add(
                "REGISTRY_MODULE",
                path,
                f"module must be {expected_module!r} for stable_id {stable_id}",
                row_number,
            )

        gameplay_fields = ("production", "uses", "hazards", "controls", "cleanup")
        if slot_is_reserved:
            for field in gameplay_fields:
                if row.get(field) != RESERVED_GAMEPLAY_SENTINEL:
                    findings.add(
                        "REGISTRY_RESERVED_GAMEPLAY",
                        path,
                        f"{field} must be {RESERVED_GAMEPLAY_SENTINEL!r} for a reserved slot",
                        row_number,
                    )
        elif stable_id <= OFFICIAL_SLOT_LAST:
            for field in gameplay_fields:
                if row.get(field) != UPSTREAM_GAMEPLAY_SENTINEL:
                    findings.add(
                        "REGISTRY_UPSTREAM_GAMEPLAY",
                        path,
                        f"{field} must be {UPSTREAM_GAMEPLAY_SENTINEL!r} for an official element",
                        row_number,
                    )
        else:
            for field in gameplay_fields:
                value = row.get(field, "").strip()
                if (
                    len(value) < 12
                    or value in {UPSTREAM_GAMEPLAY_SENTINEL, RESERVED_GAMEPLAY_SENTINEL}
                ):
                    findings.add(
                        "REGISTRY_GAMEPLAY",
                        path,
                        f"implemented OmniPack field {field} needs a concrete audited value",
                        row_number,
                    )

        if stable_id in by_id:
            findings.add("REGISTRY_DUPLICATE_ID", path, f"stable_id {stable_id} is duplicated", row_number)
        by_id[stable_id] = row
        if pt_num is not None and stable_id >= pt_num:
            findings.add(
                "REGISTRY_PT_NUM",
                path,
                f"stable_id {stable_id} is outside PT_NUM={pt_num}",
                row_number,
            )

        for field, allowed in (
            ("element_state", ELEMENT_STATES),
            ("save_compatibility", SAVE_COMPATIBILITY),
            ("implementation_status", IMPLEMENTATION_STATUSES),
            ("test_status", TEST_STATUSES),
            ("license", LICENSES),
        ):
            if row.get(field) not in allowed:
                findings.add(
                    "REGISTRY_ENUM",
                    path,
                    f"{field} has invalid value {row.get(field)!r}; expected one of {sorted(allowed)}",
                    row_number,
                )

        if row.get("menu_category") not in menu_categories | {"RESERVED"}:
            findings.add(
                "REGISTRY_MENU",
                path,
                f"invalid menu_category {row.get('menu_category')!r}",
                row_number,
            )

        if not HEX_COMMIT.fullmatch(row.get("source_commit", "")):
            findings.add("REGISTRY_COMMIT", path, "source_commit must be a 40-character lowercase hash", row_number)
        if not row.get("source_mod"):
            findings.add("REGISTRY_SOURCE", path, "source_mod is required", row_number)
        if source_id is None:
            findings.add("REGISTRY_SOURCE", path, "source_id is required", row_number)

        for field in ("english_name", "chinese_name", "english_description", "chinese_description"):
            if not row.get(field):
                findings.add("REGISTRY_TEXT", path, f"{field} is required", row_number)
        if row.get("english_name") and not ASCII_LETTER.search(row["english_name"]):
            findings.add("REGISTRY_ENGLISH", path, "english_name must contain an ASCII letter", row_number)
        if row.get("english_description") and not ASCII_LETTER.search(row["english_description"]):
            findings.add("REGISTRY_ENGLISH", path, "english_description must contain an ASCII letter", row_number)
        if row.get("chinese_name") and not CJK.search(row["chinese_name"]):
            findings.add("REGISTRY_CHINESE", path, "chinese_name must contain a CJK character", row_number)
        if row.get("chinese_description") and not CJK.search(row["chinese_description"]):
            findings.add("REGISTRY_CHINESE", path, "chinese_description must contain a CJK character", row_number)

        if slot_is_reserved:
            expected = reserved_registry_expectations(stable_id)
            for field, value in expected.items():
                if row.get(field) != value:
                    findings.add(
                        "REGISTRY_RESERVED",
                        path,
                        f"{field} must be {value!r} for reserved slot {stable_id}",
                        row_number,
                    )
            identifier_folded = row.get("identifier", "").casefold()
            if identifier_folded in identifier_owner:
                findings.add(
                    "REGISTRY_IDENTIFIER_CASE",
                    path,
                    f"identifier conflicts case-insensitively with stable_id {identifier_owner[identifier_folded]}",
                    row_number,
                )
            identifier_owner[identifier_folded] = stable_id
            continue

        identifier = row.get("identifier", "")
        display_code = row.get("display_code", "")
        meson_name = row.get("meson_name", "")
        source_file = row.get("source_file", "")
        if not ASCII_IDENTIFIER.fullmatch(identifier):
            findings.add("REGISTRY_IDENTIFIER", path, f"invalid ASCII identifier {identifier!r}", row_number)
        if not ASCII_MESON_NAME.fullmatch(meson_name):
            findings.add("REGISTRY_MESON", path, f"invalid meson_name {meson_name!r}", row_number)
        if not display_code or not ASCII_DISPLAY_CODE.fullmatch(display_code):
            findings.add("REGISTRY_DISPLAY", path, f"invalid display_code {display_code!r}", row_number)

        identifier_folded = identifier.casefold()
        if identifier_folded in identifier_owner:
            findings.add(
                "REGISTRY_IDENTIFIER_CASE",
                path,
                f"identifier conflicts case-insensitively with stable_id {identifier_owner[identifier_folded]}",
                row_number,
            )
        identifier_owner[identifier_folded] = stable_id

        display_folded = display_code.casefold()
        if display_folded in display_owner:
            findings.add(
                "REGISTRY_DISPLAY_CASE",
                path,
                f"display_code conflicts case-insensitively with stable_id {display_owner[display_folded]}",
                row_number,
            )
        display_owner[display_folded] = stable_id

        if stable_id not in lock_by_id and len(display_code) > 4:
            findings.add(
                "REGISTRY_DISPLAY_WIDTH",
                path,
                "new element display_code must contain at most four characters",
                row_number,
            )
        if "\\" in source_file or source_file.startswith("/") or ".." in Path(source_file).parts:
            findings.add("REGISTRY_SOURCE_PATH", path, "source_file must be a safe repository-relative POSIX path", row_number)
        elif not (root / source_file).is_file():
            findings.add("REGISTRY_SOURCE_FILE", path, f"source_file does not exist: {source_file}", row_number)

        if is_duplicate is True and not row.get("duplicate_of"):
            findings.add("REGISTRY_DUPLICATE", path, "duplicate_of is required when is_duplicate=true", row_number)
        if is_duplicate is False and row.get("duplicate_of"):
            findings.add("REGISTRY_DUPLICATE", path, "duplicate_of must be empty when is_duplicate=false", row_number)
        if row.get("duplicate_of", "").casefold() == identifier_folded and identifier:
            findings.add("REGISTRY_DUPLICATE", path, "an element cannot duplicate itself", row_number)

        source = sources_by_id.get(stable_id)
        if source is None:
            findings.add("REGISTRY_SOURCE_SLOT", path, f"no Meson/source element exists at stable_id {stable_id}", row_number)
        else:
            comparisons = {
                "identifier": source.identifier,
                "display_code": source.display_code,
                "meson_name": source.meson_name,
                "source_file": source.source_file,
                "menu_category": source.menu_category,
                "element_state": source.element_state,
                "default_enabled": "true" if source.enabled else "false",
            }
            for field, expected in comparisons.items():
                if row.get(field) != expected:
                    findings.add(
                        "REGISTRY_SOURCE_MISMATCH",
                        path,
                        f"{field}={row.get(field)!r}, source requires {expected!r}",
                        row_number,
                    )

            for language_name, language, name_field, text_field, separator in (
                ("en-US", english, "english_name", "english_description", ": "),
                ("zh-CN", chinese, "chinese_name", "chinese_description", "："),
            ):
                translated = language.get(source.description_key)
                language_path = root / "src" / "lang" / f"{language_name}.json"
                if not isinstance(translated, str) or not translated.strip():
                    findings.add(
                        "REGISTRY_LANGUAGE",
                        language_path,
                        f"{language_name} lacks non-empty key {source.description_key}",
                    )
                if not row.get(text_field):
                    findings.add(
                        "REGISTRY_DESCRIPTION",
                        path,
                        f"{text_field} is required for {source.identifier}",
                        row_number,
                    )

                # OmniPack descriptions are self-identifying in both source-of-
                # truth locations: users must see the element name before any
                # behaviour text, and the registry and language catalogs must
                # not silently drift apart.
                if source.identifier.startswith("OMNI_PT_"):
                    expected_name = row.get(name_field, "")
                    expected_description = row.get(text_field, "")
                    name_key = f"{source.description_key}.name"
                    localized_name = language.get(name_key)
                    if localized_name != expected_name:
                        findings.add(
                            "REGISTRY_LANGUAGE_NAME",
                            language_path,
                            f"{name_key}={localized_name!r}, registry requires "
                            f"{expected_name!r}",
                        )
                    if translated != expected_description:
                        findings.add(
                            "REGISTRY_LANGUAGE_DESCRIPTION",
                            language_path,
                            f"{source.description_key} must exactly match "
                            f"{text_field} for {source.identifier}",
                        )

                    required_prefix = f"{expected_name}{separator}"
                    if (
                        expected_name
                        and expected_description
                        and not expected_description.startswith(required_prefix)
                    ):
                        findings.add(
                            "REGISTRY_DESCRIPTION_PREFIX",
                            path,
                            f"{text_field} must start with {required_prefix!r}",
                            row_number,
                        )
                    if (
                        isinstance(translated, str)
                        and expected_name
                        and not translated.startswith(required_prefix)
                    ):
                        findings.add(
                            "REGISTRY_DESCRIPTION_PREFIX",
                            language_path,
                            f"{source.description_key} must start with {required_prefix!r}",
                        )

        official = lock_by_id.get(stable_id)
        if official is not None:
            expected_official = {
                "identifier": official["identifier"],
                "display_code": official["display_code"],
                "meson_name": official["meson_name"],
                "source_file": official["source_file"],
                "source_mod": OFFICIAL_REPOSITORY,
                "source_commit": OFFICIAL_COMMIT,
                "source_id": str(stable_id),
                "save_compatibility": "official-locked",
                "implementation_status": "implemented",
            }
            for field, expected in expected_official.items():
                if row.get(field) != expected:
                    findings.add(
                        "REGISTRY_OFFICIAL_LOCK",
                        path,
                        f"official stable_id {stable_id}: {field} must be {expected!r}",
                        row_number,
                    )

    for row in rows:
        duplicate_of = row.get("duplicate_of", "")
        if duplicate_of and duplicate_of.casefold() not in identifier_owner:
            findings.add(
                "REGISTRY_DUPLICATE_TARGET",
                path,
                f"duplicate_of target {duplicate_of!r} does not exist",
                int(row["_row_number"]),
            )

    source_ids = set(range(len(slots)))
    registry_ids = set(by_id)
    missing = sorted(source_ids - registry_ids)
    extra = sorted(registry_ids - source_ids)
    if missing:
        findings.add("REGISTRY_COVERAGE", path, f"missing source stable IDs: {missing}")
    if extra:
        findings.add("REGISTRY_COVERAGE", path, f"registry IDs absent from Meson source order: {extra}")
    return by_id


def validate_repository(
    root: Path,
    registry_path: Path,
    lock_path: Path,
) -> tuple[Findings, dict[str, int]]:
    findings = Findings(root)
    lock_rows = read_csv_file(lock_path, LOCK_COLUMNS, findings)
    registry_rows = read_csv_file(registry_path, REGISTRY_COLUMNS, findings)
    lock_by_id = validate_lock(lock_rows, lock_path, findings)

    meson_path = root / "src/simulation/elements/meson.build"
    slots = parse_meson_slots(meson_path, findings)
    pt_num = parse_pt_num(root / "src/simulation/ElementDefs.h", findings)
    menu_categories = parse_menu_categories(root / "src/simulation/MenuSection.h", findings)
    english = load_language(root / "src/lang/en-US.json", findings)
    chinese = load_language(root / "src/lang/zh-CN.json", findings)

    if pt_num is not None and len(slots) > pt_num:
        findings.add("MESON_PT_NUM", meson_path, f"{len(slots)} slots exceed PT_NUM={pt_num}")

    sources_by_id: dict[int, ElementSource] = {}
    source_identifier_owner: dict[str, int] = {}
    source_display_owner: dict[str, int] = {}
    for stable_id, meson_name in enumerate(slots):
        lock = lock_by_id.get(stable_id)
        if meson_name is None:
            # Every unimplemented OmniPack content slot is explicitly
            # represented in the registry as a reserved compatibility gap.
            # This permits later modules to claim fixed IDs without shifting
            # any earlier element, while still rejecting undocumented holes.
            is_known_reserved = (
                stable_id == OFFICIAL_RESERVED_SLOT
                or OMNI_RESERVED_FIRST <= stable_id < pt_num
            )
            if not is_known_reserved:
                findings.add("MESON_RESERVED", meson_path, f"unexpected disabled slot {stable_id}")
            if lock and lock.get("slot_status") != "reserved":
                findings.add("MESON_LOCK", meson_path, f"slot {stable_id} is disabled but lock is not reserved")
            continue

        if not ASCII_MESON_NAME.fullmatch(meson_name):
            findings.add("MESON_NAME", meson_path, f"slot {stable_id} has invalid name {meson_name!r}")
            continue
        source = parse_element_source(root, stable_id, meson_name, findings)
        if source is None:
            continue
        sources_by_id[stable_id] = source

        identifier_folded = source.identifier.casefold()
        if identifier_folded in source_identifier_owner:
            findings.add(
                "SOURCE_IDENTIFIER_CASE",
                root / source.source_file,
                f"identifier conflicts case-insensitively with slot {source_identifier_owner[identifier_folded]}",
            )
        source_identifier_owner[identifier_folded] = stable_id
        display_folded = source.display_code.casefold()
        if display_folded in source_display_owner:
            findings.add(
                "SOURCE_DISPLAY_CASE",
                root / source.source_file,
                f"display code conflicts case-insensitively with slot {source_display_owner[display_folded]}",
            )
        source_display_owner[display_folded] = stable_id

        if lock is not None:
            comparisons = {
                "meson_name": source.meson_name,
                "identifier": source.identifier,
                "display_code": source.display_code,
                "source_file": source.source_file,
            }
            for field, actual in comparisons.items():
                if lock.get(field) != actual:
                    findings.add(
                        "OFFICIAL_SOURCE_DRIFT",
                        root / source.source_file,
                        f"slot {stable_id} {field}={actual!r}, official lock requires {lock.get(field)!r}",
                    )

    if lock_by_id and slots:
        for stable_id in range(OFFICIAL_SLOT_FIRST, OFFICIAL_SLOT_LAST + 1):
            if stable_id >= len(slots):
                findings.add("OFFICIAL_SOURCE_DRIFT", meson_path, f"official slot {stable_id} is missing")
                continue
            lock = lock_by_id.get(stable_id)
            if lock is None:
                continue
            expected_meson = None if stable_id == OFFICIAL_RESERVED_SLOT else lock["meson_name"]
            if slots[stable_id] != expected_meson:
                findings.add(
                    "OFFICIAL_SOURCE_DRIFT",
                    meson_path,
                    f"slot {stable_id} is {slots[stable_id]!r}, lock requires {expected_meson!r}",
                )

    validate_registry(
        registry_rows,
        registry_path,
        root,
        lock_by_id,
        sources_by_id,
        slots,
        pt_num,
        menu_categories,
        english,
        chinese,
        findings,
    )

    stats = {
        "pt_num": pt_num or 0,
        "slots": len(slots),
        "active": sum(slot is not None for slot in slots),
        "reserved": sum(slot is None for slot in slots),
        "registry_rows": len(registry_rows),
        "lock_rows": len(lock_rows),
        "errors": len(findings.errors),
    }
    return findings, stats


def run_self_test() -> list[str]:
    failures: list[str] = []

    def expect(condition: bool, label: str) -> None:
        if not condition:
            failures.append(label)

    expect(eval_numeric_expression("273.15f + 100.0f") == 373.15, "numeric expression")
    expect(eval_numeric_expression("1.0f / 0.0f") is None, "division by zero rejection")
    expect(eval_numeric_expression("unknown + 1") is None, "unknown identifier rejection")
    expect(bool(ASCII_IDENTIFIER.fullmatch("DEFAULT_PT_WATR")), "valid identifier")
    expect(not ASCII_IDENTIFIER.fullmatch("default_pt_watr"), "lowercase identifier rejection")
    expect(bool(CJK.search("简体中文")), "CJK detection")
    expect(not CJK.search("English only"), "non-CJK rejection")
    reserved = reserved_registry_expectations(196)
    expect(reserved["identifier"] == "RESERVED_PT_196", "reserved identifier")
    expect(reserved["source_file"] == "", "reserved source is empty")
    expect(reserved["implementation_status"] == "reserved", "reserved status")

    sample = [
        {
            column: value
            for column, value in zip(
                LOCK_COLUMNS,
                (
                    OFFICIAL_COMMIT,
                    OFFICIAL_VERSION,
                    "0",
                    "active",
                    "NONE",
                    "DEFAULT_PT_NONE",
                    "NONE",
                    "src/simulation/elements/NONE.cpp",
                ),
            )
        }
    ]
    expect(canonical_lock_digest(sample) == canonical_lock_digest(list(reversed(sample))), "stable digest")

    local_findings = Findings(Path("."))
    malformed = ",".join(REGISTRY_COLUMNS) + "\nonly-one-field\n"
    parse_csv_text(malformed, Path("fixture.csv"), REGISTRY_COLUMNS, local_findings)
    expect(any(item.code == "CSV_WIDTH" for item in local_findings.errors), "CSV width rejection")
    return failures


def resolve_input(root: Path, value: str | None, default: str) -> Path:
    if value is None:
        return root / default
    candidate = Path(value)
    return candidate if candidate.is_absolute() else root / candidate


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        help="repository root (default: parent of this script's tools directory)",
    )
    parser.add_argument("--registry", help="registry CSV path, relative to --root by default")
    parser.add_argument("--official-lock", help="official lock CSV path, relative to --root by default")
    parser.add_argument("--json", action="store_true", help="emit deterministic JSON")
    parser.add_argument("--quiet", action="store_true", help="suppress PASS output")
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="run in-memory parser checks only; never writes files",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.self_test:
        failures = run_self_test()
        if args.json:
            print(json.dumps({"mode": "self-test", "failures": failures}, ensure_ascii=False, sort_keys=True))
        elif failures:
            for failure in failures:
                print(f"element-registry-check: SELF-TEST FAIL: {failure}", file=sys.stderr)
        elif not args.quiet:
            print("element-registry-check: SELF-TEST PASS (12 checks, no files modified)")
        return 1 if failures else 0

    default_root = Path(__file__).resolve().parents[1]
    root = Path(args.root).resolve() if args.root else default_root
    registry_path = resolve_input(root, args.registry, "docs/ELEMENT_REGISTRY.csv")
    lock_path = resolve_input(root, args.official_lock, "tools/data/official_elements_100_0.csv")

    findings, stats = validate_repository(root, registry_path, lock_path)
    if args.json:
        payload = {
            "status": "pass" if not findings.errors else "fail",
            "stats": stats,
            "errors": [
                {
                    "code": item.code,
                    "path": item.path,
                    "row": item.row,
                    "message": item.message,
                }
                for item in findings.errors
            ],
        }
        print(json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":")))
    elif findings.errors:
        for item in findings.errors:
            print(item.text(), file=sys.stderr)
        print(
            f"element-registry-check: FAIL ({len(findings.errors)} errors)",
            file=sys.stderr,
        )
    elif not args.quiet:
        print(
            "element-registry-check: PASS "
            f"({stats['slots']} slots, {stats['active']} active, "
            f"{stats['reserved']} reserved, PT_NUM={stats['pt_num']}, "
            f"registry={stats['registry_rows']}, lock={stats['lock_rows']})"
        )
    return 1 if findings.errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
