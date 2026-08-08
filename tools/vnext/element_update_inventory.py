#!/usr/bin/env python3
"""Generate a conservative lexical inventory of built-in element updates.

This tool is intentionally read-only with respect to production sources.  It
parses the Meson element registry, resolves constructor Update bindings, and
scans each resolved update root plus same-file and globally unique bare helper
functions.
It is not a C++ parser: every result is evidence for later AST/data-flow review,
not a proof that an access or dependency is absent.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


SCHEMA_VERSION = "1.0.0"
GENERATOR_VERSION = "1.0.0"
GENERATOR_PATH = "tools/vnext/element_update_inventory.py"


@dataclass(frozen=True)
class RegistryEntry:
    stable_id: int
    name: str


@dataclass(frozen=True)
class FunctionDef:
    path: str
    full_name: str
    short_name: str
    signature_offset: int
    body_offset: int
    body_end: int
    signature_line: int
    text: str
    masked: str

    @property
    def body(self) -> str:
        return self.text[self.body_offset : self.body_end]

    @property
    def masked_body(self) -> str:
        return self.masked[self.body_offset : self.body_end]

    def line_for_body_offset(self, offset: int) -> int:
        return self.text.count("\n", 0, self.body_offset + offset) + 1


@dataclass(frozen=True)
class ConstructorInfo:
    identifier: str
    display_name: str
    direct_update: str | None
    configurators: tuple[str, ...]


def mask_cpp(text: str) -> str:
    """Blank comments and literals while preserving byte offsets/newlines."""

    output = list(text)
    state = "code"
    quote = ""
    escaped = False
    index = 0
    while index < len(text):
        char = text[index]
        nxt = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and nxt == "/":
                output[index] = output[index + 1] = " "
                state = "line_comment"
                index += 2
                continue
            if char == "/" and nxt == "*":
                output[index] = output[index + 1] = " "
                state = "block_comment"
                index += 2
                continue
            if char in ('"', "'"):
                quote = char
                output[index] = " "
                state = "literal"
                escaped = False
                index += 1
                continue
        elif state == "line_comment":
            if char == "\n":
                state = "code"
            else:
                output[index] = " "
            index += 1
            continue
        elif state == "block_comment":
            if char == "*" and nxt == "/":
                output[index] = output[index + 1] = " "
                state = "code"
                index += 2
                continue
            if char != "\n":
                output[index] = " "
            index += 1
            continue
        elif state == "literal":
            if char != "\n":
                output[index] = " "
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                state = "code"
            index += 1
            continue
        index += 1
    return "".join(output)


def matching_delimiter(masked: str, start: int, opening: str, closing: str) -> int:
    if start >= len(masked) or masked[start] != opening:
        raise ValueError(f"expected {opening!r} at offset {start}")
    depth = 0
    for index in range(start, len(masked)):
        char = masked[index]
        if char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return index
    raise ValueError(f"unbalanced {opening}{closing} at offset {start}")


FUNCTION_HEAD = re.compile(
    r"(?m)^[ \t]*"
    r"(?:(?:static|inline|constexpr|extern|virtual)\s+)*"
    r"[A-Za-z_][A-Za-z0-9_:<>,*& \t]*?[ \t]+"
    r"(?P<name>(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)[ \t]*\("
)
NON_FUNCTION_NAMES = frozenset({"if", "for", "while", "switch", "catch"})


def extract_functions(path: str, text: str) -> list[FunctionDef]:
    masked = mask_cpp(text)
    functions: list[FunctionDef] = []
    for match in FUNCTION_HEAD.finditer(masked):
        open_paren = masked.find("(", match.start("name") + len(match.group("name")))
        try:
            close_paren = matching_delimiter(masked, open_paren, "(", ")")
        except ValueError:
            continue
        cursor = close_paren + 1
        while cursor < len(masked) and masked[cursor].isspace():
            cursor += 1
        # Free update/configuration functions in this codebase have no trailing
        # qualifiers.  Skip declarations and expressions rather than guessing.
        if cursor >= len(masked) or masked[cursor] != "{":
            continue
        try:
            close_brace = matching_delimiter(masked, cursor, "{", "}")
        except ValueError:
            continue
        full_name = match.group("name")
        if full_name.rsplit("::", 1)[-1] in NON_FUNCTION_NAMES:
            continue
        functions.append(
            FunctionDef(
                path=path,
                full_name=full_name,
                short_name=full_name.rsplit("::", 1)[-1],
                signature_offset=match.start(),
                body_offset=cursor + 1,
                body_end=close_brace,
                signature_line=text.count("\n", 0, match.start()) + 1,
                text=text,
                masked=masked,
            )
        )
    return functions


def extract_named_body(text: str, signature_pattern: re.Pattern[str]) -> tuple[str, str]:
    masked = mask_cpp(text)
    match = signature_pattern.search(masked)
    if not match:
        raise ValueError(f"definition not found: {signature_pattern.pattern}")
    open_brace = masked.find("{", match.end())
    if open_brace < 0:
        raise ValueError(f"body not found: {signature_pattern.pattern}")
    close_brace = matching_delimiter(masked, open_brace, "{", "}")
    return text[open_brace + 1 : close_brace], masked[open_brace + 1 : close_brace]


def parse_registry(path: Path) -> list[RegistryEntry]:
    entries: list[RegistryEntry] = []
    in_list = False
    stable_id = 0
    for line in path.read_text(encoding="utf-8").splitlines():
        if not in_list:
            if re.match(r"\s*simulation_elem_names\s*=\s*\[", line):
                in_list = True
            continue
        if re.match(r"\s*\]", line):
            break
        name_match = re.match(r"\s*'([A-Z0-9_]+)'\s*,", line)
        disabled_match = re.match(r"\s*disabler\(\)\s*,", line)
        if name_match:
            entries.append(RegistryEntry(stable_id=stable_id, name=name_match.group(1)))
            stable_id += 1
        elif disabled_match:
            stable_id += 1
    if not in_list or not entries:
        raise ValueError(f"could not parse element registry: {path}")
    return entries


def parse_constructor(text: str, element_name: str) -> ConstructorInfo:
    pattern = re.compile(
        rf"\bvoid\s+Element::Element_{re.escape(element_name)}\s*\(\s*\)"
    )
    body, masked_body = extract_named_body(text, pattern)
    identifier_match = re.search(r'\bIdentifier\s*=\s*"([^"]+)"\s*;', body)
    name_match = re.search(r'\bName\s*=\s*"([^"]+)"\s*;', body)
    if not identifier_match or not name_match:
        raise ValueError(f"missing Identifier/Name in Element_{element_name}")
    direct_match = re.search(
        r"\bUpdate\s*=\s*&\s*([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*;",
        masked_body,
    )
    configurators = tuple(
        sorted(
            set(
                re.findall(
                    r"\b([A-Za-z_]\w*)\s*\(\s*\*\s*this\b", masked_body
                )
            )
        )
    )
    return ConstructorInfo(
        identifier=identifier_match.group(1),
        display_name=name_match.group(1),
        direct_update=direct_match.group(1) if direct_match else None,
        configurators=configurators,
    )


def bare_call_names(masked_body: str) -> set[str]:
    calls: set[str] = set()
    for match in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", masked_body):
        prefix = masked_body[max(0, match.start() - 2) : match.start()]
        if prefix.endswith("->") or prefix.endswith("::") or prefix.endswith("."):
            continue
        calls.add(match.group(1))
    return calls


def reachable_functions(
    root: FunctionDef,
    functions_by_path: dict[str, list[FunctionDef]],
    functions_by_name: dict[str, list[FunctionDef]],
) -> list[FunctionDef]:
    selected: dict[tuple[str, int], FunctionDef] = {}
    pending = [root]
    while pending:
        function = pending.pop()
        key = (function.path, function.signature_offset)
        if key in selected:
            continue
        selected[key] = function
        local_by_name: dict[str, list[FunctionDef]] = {}
        for candidate in functions_by_path[function.path]:
            local_by_name.setdefault(candidate.short_name, []).append(candidate)
        calls = bare_call_names(function.masked_body)
        for call in sorted(calls, reverse=True):
            candidates = local_by_name.get(call, [])
            if not candidates and len(functions_by_name.get(call, [])) == 1:
                candidates = functions_by_name[call]
            for candidate in reversed(candidates):
                candidate_key = (candidate.path, candidate.signature_offset)
                if candidate_key not in selected:
                    pending.append(candidate)
    return sorted(selected.values(), key=lambda item: (item.path, item.signature_offset))


def resolve_function(
    symbol: str,
    preferred_path: str,
    functions_by_path: dict[str, list[FunctionDef]],
    functions_by_name: dict[str, list[FunctionDef]],
) -> tuple[FunctionDef | None, str]:
    short_symbol = symbol.rsplit("::", 1)[-1]
    local = [
        function
        for function in functions_by_path.get(preferred_path, [])
        if function.short_name == short_symbol
    ]
    if len(local) == 1:
        return local[0], "source-local"
    global_matches = functions_by_name.get(short_symbol, [])
    if len(global_matches) == 1:
        return global_matches[0], "global-unique"
    if not global_matches:
        return None, "unresolved-not-found"
    return None, f"unresolved-ambiguous:{len(global_matches)}"


def resolve_configurator_update(
    configurator: str,
    functions_by_path: dict[str, list[FunctionDef]],
    functions_by_name: dict[str, list[FunctionDef]],
) -> tuple[str | None, FunctionDef | None, str]:
    candidates = functions_by_name.get(configurator, [])
    if len(candidates) != 1:
        reason = "not-found" if not candidates else f"ambiguous:{len(candidates)}"
        return None, None, f"configurator-{reason}"
    config_root = candidates[0]
    closure = reachable_functions(config_root, functions_by_path, functions_by_name)
    symbols: set[str] = set()
    for function in closure:
        symbols.update(
            re.findall(
                r"\b[A-Za-z_]\w*\s*\.\s*Update\s*=\s*&\s*"
                r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*;",
                function.masked_body,
            )
        )
    if len(symbols) != 1:
        return None, config_root, f"configurator-update-symbols:{len(symbols)}"
    return next(iter(symbols)), config_root, "configurator-reachable-assignment"


def access_mode(line: str, end: int, start: int) -> tuple[bool, bool]:
    after = line[end:]
    before = line[:start]
    operator_match = re.match(r"\s*(\+\+|--|\+=|-=|\*=|/=|%=|=(?!=))", after)
    prefix = bool(re.search(r"(?:\+\+|--)\s*$", before))
    if prefix:
        return True, True
    if not operator_match:
        return True, False
    operator = operator_match.group(1)
    if operator == "=":
        return False, True
    return True, True


def source_evidence(
    functions: Sequence[FunctionDef], pattern: re.Pattern[str], limit: int = 12
) -> tuple[int, list[dict[str, object]]]:
    total = 0
    evidence: list[dict[str, object]] = []
    for function in functions:
        for match in pattern.finditer(function.masked_body):
            total += 1
            if len(evidence) >= limit:
                continue
            line_start = function.body.rfind("\n", 0, match.start()) + 1
            line_end = function.body.find("\n", match.end())
            if line_end < 0:
                line_end = len(function.body)
            snippet = function.body[line_start:line_end].strip()
            evidence.append(
                {
                    "path": function.path,
                    "line": function.line_for_body_offset(match.start()),
                    "function": function.short_name,
                    "text": snippet[:240],
                }
            )
    return total, evidence


def operation_observation(
    functions: Sequence[FunctionDef], pattern: str
) -> dict[str, object]:
    count, evidence = source_evidence(functions, re.compile(pattern))
    return {"detected": bool(count), "static_call_sites": count, "evidence": evidence}


def array_observation(
    functions: Sequence[FunctionDef], pattern: re.Pattern[str]
) -> dict[str, object]:
    reads = 0
    writes = 0
    evidence: list[dict[str, object]] = []
    for function in functions:
        for line_index, (raw_line, masked_line) in enumerate(
            zip(function.body.splitlines(), function.masked_body.splitlines())
        ):
            for match in pattern.finditer(masked_line):
                is_read, is_write = access_mode(masked_line, match.end(), match.start())
                reads += int(is_read)
                writes += int(is_write)
                if len(evidence) < 12:
                    evidence.append(
                        {
                            "path": function.path,
                            "line": function.line_for_body_offset(
                                sum(len(item) + 1 for item in function.body.splitlines()[:line_index])
                                + match.start()
                            ),
                            "function": function.short_name,
                            "mode": (
                                "read_write"
                                if is_read and is_write
                                else "write"
                                if is_write
                                else "read"
                            ),
                            "text": raw_line.strip()[:240],
                        }
                    )
    return {
        "read_detected": bool(reads),
        "write_detected": bool(writes),
        "read_sites": reads,
        "write_sites": writes,
        "evidence": evidence,
    }


PART_ACCESS = re.compile(
    r"\bparts\s*\[\s*(?P<index>[^\]\n]+?)\s*\]"
    r"(?:\s*\.\s*(?P<field>[A-Za-z_]\w*))?"
)


def particle_access_observation(functions: Sequence[FunctionDef]) -> dict[str, object]:
    groups = {
        "self": {"read_fields": set(), "written_fields": set(), "evidence": []},
        "non_self": {"read_fields": set(), "written_fields": set(), "evidence": []},
    }
    aliases: list[dict[str, object]] = []
    for function in functions:
        raw_lines = function.body.splitlines()
        masked_lines = function.masked_body.splitlines()
        offsets: list[int] = []
        cursor = 0
        for raw_line in raw_lines:
            offsets.append(cursor)
            cursor += len(raw_line) + 1
        for line_index, (raw_line, masked_line) in enumerate(zip(raw_lines, masked_lines)):
            for match in PART_ACCESS.finditer(masked_line):
                index = re.sub(r"\s+", "", match.group("index"))
                group_name = "self" if index == "i" else "non_self"
                group = groups[group_name]
                field = match.group("field") or "*"
                is_read, is_write = access_mode(masked_line, match.end(), match.start())
                if is_read:
                    group["read_fields"].add(field)
                if is_write:
                    group["written_fields"].add(field)
                if len(group["evidence"]) < 16:
                    group["evidence"].append(
                        {
                            "path": function.path,
                            "line": function.line_for_body_offset(
                                offsets[line_index] + match.start()
                            ),
                            "function": function.short_name,
                            "index": match.group("index").strip(),
                            "field": field,
                            "mode": (
                                "read_write"
                                if is_read and is_write
                                else "write"
                                if is_write
                                else "read"
                            ),
                            "text": raw_line.strip()[:240],
                        }
                    )
            if re.search(
                r"\b(?:auto|Particle)\s*&\s*[A-Za-z_]\w*\s*=\s*parts\s*\[",
                masked_line,
            ) and len(aliases) < 12:
                aliases.append(
                    {
                        "path": function.path,
                        "line": function.line_for_body_offset(offsets[line_index]),
                        "function": function.short_name,
                        "text": raw_line.strip()[:240],
                    }
                )
    result: dict[str, object] = {}
    for name, group in groups.items():
        result[name] = {
            "read_fields": sorted(group["read_fields"]),
            "written_fields": sorted(group["written_fields"]),
            "read_detected": bool(group["read_fields"]),
            "write_detected": bool(group["written_fields"]),
            "evidence": group["evidence"],
        }
    result["aliasing_detected"] = bool(aliases)
    result["alias_evidence"] = aliases
    result["scope_note"] = (
        "self means literal parts[i]; non_self means any other lexical index and may "
        "represent a neighbour, spawned particle, portal particle, or remote particle"
    )
    return result


def analyze_implementation(functions: Sequence[FunctionDef]) -> dict[str, object]:
    particle = particle_access_observation(functions)
    pmap = array_observation(
        functions,
        re.compile(r"\b(?:sim\s*->\s*)?pmap\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]"),
    )
    photons = array_observation(
        functions,
        re.compile(r"\b(?:sim\s*->\s*)?photons\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]"),
    )
    pressure = array_observation(
        functions, re.compile(r"\bsim\s*->\s*pv\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]")
    )
    air_vx = array_observation(
        functions, re.compile(r"\bsim\s*->\s*vx\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]")
    )
    air_vy = array_observation(
        functions, re.compile(r"\bsim\s*->\s*vy\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]")
    )
    air_heat = array_observation(
        functions, re.compile(r"\bsim\s*->\s*hv\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]")
    )
    gravity = array_observation(
        functions,
        re.compile(
            r"\bsim\s*->\s*(?:gravIn|gravOut)\s*\.\s*"
            r"(?:mass|mask|forceX|forceY)\s*\[[^\]\n]+\]"
        ),
    )
    gravity_calls = operation_observation(
        functions, r"\bsim\s*->\s*GetGravityField\s*\("
    )
    emap = array_observation(
        functions, re.compile(r"\bsim\s*->\s*emap\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]")
    )
    wireless = array_observation(
        functions,
        re.compile(r"\bsim\s*->\s*wireless\s*\[[^\]\n]+\]\s*\[[^\]\n]+\]"),
    )

    operations = {
        "create_part": operation_observation(functions, r"\bsim\s*->\s*create_part\s*\("),
        "kill_part": operation_observation(functions, r"\bsim\s*->\s*kill_part\s*\("),
        "part_change_type": operation_observation(
            functions, r"\bsim\s*->\s*part_change_type\s*\("
        ),
        "movement": operation_observation(
            functions, r"\bsim\s*->\s*(?:move|try_move|eval_move)\s*\("
        ),
        "swap": operation_observation(
            functions, r"\b(?:sim\s*->\s*[A-Za-z_]*swap[A-Za-z_]*|std::swap)\s*\("
        ),
        "flood_or_bulk": operation_observation(
            functions, r"\b(?:[A-Za-z_]*Flood[A-Za-z_]*|flood_[A-Za-z_]\w*)\s*\("
        ),
    }
    rng = operation_observation(
        functions, r"\b(?:sim\s*->\s*)?rng\s*(?:\.|\()"
    )
    electricity_symbols = operation_observation(
        functions,
        r"\b(?:PT_SPRK|PROP_CONDUCTS|conductTo|spark|wireless|ISWIRE|emap)\b",
    )
    long_range = operation_observation(
        functions,
        r"\b(?:[A-Za-z_]*Flood[A-Za-z_]*|flood_[A-Za-z_]\w*|portalp?|wireless|"
        r"xCopyTo|yCopyTo|targetX|targetY|rayLength|radius|distance)\b",
    )

    non_self = particle["non_self"]
    self_access = particle["self"]
    position_write = bool(
        {"x", "y"}.intersection(self_access["written_fields"])
        or {"x", "y"}.intersection(non_self["written_fields"])
    )
    electricity_read = bool(
        emap["read_detected"]
        or wireless["read_detected"]
        or electricity_symbols["detected"]
    )
    electricity_write = bool(
        emap["write_detected"]
        or wireless["write_detected"]
        or re.search(
            r"\bsim\s*->\s*ISWIRE\s*(?:\+\+|--|[+\-*/]?=)",
            "\n".join(function.masked_body for function in functions),
        )
    )

    high_flags: list[str] = []
    medium_flags: list[str] = []
    for name in ("create_part", "kill_part", "part_change_type", "movement", "swap"):
        if operations[name]["detected"]:
            high_flags.append(name)
    if operations["flood_or_bulk"]["detected"]:
        high_flags.append("flood_or_bulk")
    if non_self["write_detected"]:
        high_flags.append("non_self_particle_write")
    if pmap["write_detected"] or photons["write_detected"]:
        high_flags.append("occupancy_map_write")
    if any(
        field["write_detected"]
        for field in (pressure, air_vx, air_vy, air_heat, gravity)
    ):
        high_flags.append("global_field_write")
    if electricity_write:
        high_flags.append("electric_state_write")
    if long_range["detected"]:
        high_flags.append("possible_long_range_access")
    if position_write:
        high_flags.append("particle_position_write")
    if rng["detected"]:
        medium_flags.append("rng")
    if non_self["read_detected"]:
        medium_flags.append("non_self_particle_read")
    if pmap["read_detected"] or photons["read_detected"]:
        medium_flags.append("occupancy_map_read")
    if electricity_read:
        medium_flags.append("electric_interaction")

    risk_level = "high" if high_flags else "medium" if medium_flags else "unknown"
    same_frame_possible = bool(
        non_self["read_detected"]
        or pmap["read_detected"]
        or photons["read_detected"]
        or pressure["read_detected"]
        or air_vx["read_detected"]
        or air_vy["read_detected"]
        or air_heat["read_detected"]
        or electricity_read
    )
    iteration_possible = bool(high_flags)

    return {
        "analysis_complete": False,
        "analysis_scope": (
            "root plus lexical same-file and globally unique bare-function call-graph closure"
        ),
        "particle_access": particle,
        "maps": {"pmap": pmap, "photons": photons},
        "operations": operations,
        "fields": {
            "pressure_pv": pressure,
            "air_velocity_vx": air_vx,
            "air_velocity_vy": air_vy,
            "air_heat_hv": air_heat,
            "gravity_state": gravity,
            "gravity_query": gravity_calls,
            "electricity": {
                "read_detected": electricity_read,
                "write_detected": electricity_write,
                "emap": emap,
                "wireless": wireless,
                "symbol_evidence": electricity_symbols,
            },
        },
        "dependencies": {
            "rng": rng,
            "long_range_read": {
                "status": "possible" if long_range["detected"] else "unknown",
                "lexical_evidence": long_range,
            },
            "iteration_order_dependency": {
                "status": "possible" if iteration_possible else "unknown",
                "basis": sorted(set(high_flags)),
            },
            "same_frame_dependency": {
                "status": "possible" if same_frame_possible else "unknown",
                "basis": (
                    "non-self/map/global reads detected"
                    if same_frame_possible
                    else "no direct lexical evidence; transitive/runtime effects remain unanalysed"
                ),
            },
        },
        "gpu_risk": {
            "lexical_level": risk_level,
            "high_risk_flags": sorted(set(high_flags)),
            "medium_risk_flags": sorted(set(medium_flags)),
            "not_a_gpu_classification": True,
        },
    }


def compact_observations(analysis: dict[str, object]) -> dict[str, object]:
    particle = analysis["particle_access"]
    maps = analysis["maps"]
    operations = analysis["operations"]
    fields = analysis["fields"]
    dependencies = analysis["dependencies"]
    return {
        "analysis_complete": False,
        "self_reads": particle["self"]["read_fields"],
        "self_writes": particle["self"]["written_fields"],
        "non_self_reads": particle["non_self"]["read_fields"],
        "non_self_writes": particle["non_self"]["written_fields"],
        "pmap_read": maps["pmap"]["read_detected"],
        "pmap_write": maps["pmap"]["write_detected"],
        "photons_read": maps["photons"]["read_detected"],
        "photons_write": maps["photons"]["write_detected"],
        "create_part": operations["create_part"]["detected"],
        "kill_part": operations["kill_part"]["detected"],
        "part_change_type": operations["part_change_type"]["detected"],
        "move": operations["movement"]["detected"],
        "swap": operations["swap"]["detected"],
        "pressure_read": fields["pressure_pv"]["read_detected"],
        "pressure_write": fields["pressure_pv"]["write_detected"],
        "air_velocity_read": bool(
            fields["air_velocity_vx"]["read_detected"]
            or fields["air_velocity_vy"]["read_detected"]
        ),
        "air_velocity_write": bool(
            fields["air_velocity_vx"]["write_detected"]
            or fields["air_velocity_vy"]["write_detected"]
        ),
        "air_heat_read": fields["air_heat_hv"]["read_detected"],
        "air_heat_write": fields["air_heat_hv"]["write_detected"],
        "gravity_read": bool(
            fields["gravity_state"]["read_detected"]
            or fields["gravity_query"]["detected"]
        ),
        "gravity_write": fields["gravity_state"]["write_detected"],
        "electricity_read": fields["electricity"]["read_detected"],
        "electricity_write": fields["electricity"]["write_detected"],
        "rng": dependencies["rng"]["detected"],
        "long_range_read": dependencies["long_range_read"]["status"],
        "iteration_order_dependency": dependencies["iteration_order_dependency"]["status"],
        "same_frame_dependency": dependencies["same_frame_dependency"]["status"],
        "gpu_lexical_risk": analysis["gpu_risk"]["lexical_level"],
        "gpu_risk_flags": analysis["gpu_risk"]["high_risk_flags"],
    }


def input_manifest(root: Path, paths: Iterable[Path]) -> tuple[str, list[dict[str, object]]]:
    records: list[dict[str, object]] = []
    aggregate = hashlib.sha256()
    unique_paths = sorted({path.resolve() for path in paths}, key=lambda item: item.as_posix())
    for path in unique_paths:
        relative = path.relative_to(root.resolve()).as_posix()
        payload = canonical_lf(path.read_bytes())
        digest = hashlib.sha256(payload).hexdigest()
        records.append(
            {
                "path": relative,
                "canonical_lf_bytes": len(payload),
                "sha256": digest,
            }
        )
        aggregate.update(relative.encode("utf-8"))
        aggregate.update(b"\0")
        aggregate.update(digest.encode("ascii"))
        aggregate.update(b"\n")
    return aggregate.hexdigest(), records


def build_inventory(root: Path, baseline_commit: str) -> dict[str, object]:
    registry_path = root / "src/simulation/elements/meson.build"
    entries = parse_registry(registry_path)
    simulation_root = root / "src/simulation"
    cpp_paths = sorted(simulation_root.rglob("*.cpp"), key=lambda item: item.as_posix())
    source_text: dict[str, str] = {}
    functions_by_path: dict[str, list[FunctionDef]] = {}
    functions_by_name: dict[str, list[FunctionDef]] = {}
    for path in cpp_paths:
        relative = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8")
        source_text[relative] = text
        functions = extract_functions(relative, text)
        functions_by_path[relative] = functions
        for function in functions:
            functions_by_name.setdefault(function.short_name, []).append(function)

    parsed: list[tuple[RegistryEntry, str, ConstructorInfo]] = []
    for entry in entries:
        relative = f"src/simulation/elements/{entry.name}.cpp"
        if relative not in source_text:
            raise ValueError(f"registered source is missing: {relative}")
        parsed.append((entry, relative, parse_constructor(source_text[relative], entry.name)))

    root_records: dict[str, dict[str, object]] = {}
    element_records: list[dict[str, object]] = []
    unresolved: list[dict[str, object]] = []
    direct_bindings = 0
    configurator_bindings = 0

    for entry, source_path, constructor in parsed:
        symbol: str | None = constructor.direct_update
        binding_mode = "direct" if symbol else "none"
        binding_evidence = "constructor Update assignment" if symbol else "no Update binding found"
        if symbol:
            direct_bindings += 1
        elif constructor.configurators:
            resolved_configurations: list[tuple[str, str, FunctionDef, str]] = []
            for configurator in constructor.configurators:
                configured_symbol, config_root, config_reason = resolve_configurator_update(
                    configurator, functions_by_path, functions_by_name
                )
                if configured_symbol and config_root:
                    resolved_configurations.append(
                        (configurator, configured_symbol, config_root, config_reason)
                    )
            unique_symbols = sorted({item[1] for item in resolved_configurations})
            if len(unique_symbols) == 1:
                symbol = unique_symbols[0]
                binding_mode = "configurator"
                binding_evidence = ", ".join(
                    f"{item[0]} via {item[3]}" for item in resolved_configurations
                )
                configurator_bindings += 1
            elif resolved_configurations:
                binding_mode = "unresolved-configurator"
                binding_evidence = f"multiple configured Update symbols: {unique_symbols}"

        implementation_root_id: str | None = None
        compact: dict[str, object] | None = None
        resolution = "not-bound"
        if symbol:
            root_function, resolution = resolve_function(
                symbol, source_path, functions_by_path, functions_by_name
            )
            if root_function:
                closure = reachable_functions(
                    root_function, functions_by_path, functions_by_name
                )
                implementation_root_id = (
                    f"{root_function.path}:{root_function.signature_line}:{root_function.short_name}"
                )
                if implementation_root_id not in root_records:
                    analysis = analyze_implementation(closure)
                    root_records[implementation_root_id] = {
                        "root_id": implementation_root_id,
                        "symbol": root_function.short_name,
                        "source": root_function.path,
                        "line": root_function.signature_line,
                        "resolution": resolution,
                        "analysis_complete": False,
                        "reachable_function_count": len(closure),
                        "reachable_functions": [
                            {
                                "name": function.short_name,
                                "source": function.path,
                                "line": function.signature_line,
                            }
                            for function in closure
                        ],
                        "analysis": analysis,
                    }
                compact = compact_observations(root_records[implementation_root_id]["analysis"])
            else:
                unresolved.append(
                    {
                        "id": entry.stable_id,
                        "name": entry.name,
                        "source": source_path,
                        "symbol": symbol,
                        "reason": resolution,
                    }
                )

        origin = (
            "upstream"
            if constructor.identifier.startswith("DEFAULT_PT_")
            else "omnipack"
            if constructor.identifier.startswith("OMNI_PT_")
            else "unknown"
        )
        element_records.append(
            {
                "id": entry.stable_id,
                "name": entry.name,
                "identifier": constructor.identifier,
                "display_name": constructor.display_name,
                "origin": origin,
                "source": source_path,
                "custom_update_bound": bool(symbol),
                "update_binding": {
                    "mode": binding_mode,
                    "symbol": symbol,
                    "resolution": resolution,
                    "evidence": binding_evidence,
                    "configurators": list(constructor.configurators),
                },
                "implementation_root_id": implementation_root_id,
                "classification": "UNKNOWN",
                "classification_reason": (
                    "AST/data-flow/manual semantic review not completed; lexical evidence "
                    "must not be promoted to GENERIC/RULE_DRIVEN/SPECIAL classifications"
                ),
                "analysis_complete": False,
                "observations": compact,
            }
        )

    root_list = sorted(root_records.values(), key=lambda item: item["root_id"])
    sharing: dict[str, list[int]] = {}
    for element in element_records:
        root_id = element["implementation_root_id"]
        if root_id:
            sharing.setdefault(root_id, []).append(element["id"])
    for root_record in root_list:
        ids = sharing[root_record["root_id"]]
        root_record["element_count"] = len(ids)
        root_record["element_ids"] = ids
        root_record["shared_by_multiple_elements"] = len(ids) > 1

    custom_elements = [item for item in element_records if item["custom_update_bound"]]
    no_update_elements = [item for item in element_records if not item["custom_update_bound"]]
    resolved_elements = [item for item in custom_elements if item["implementation_root_id"]]
    risk_counts = {"high": 0, "medium": 0, "unknown": 0}
    root_risk_counts = {"high": 0, "medium": 0, "unknown": 0}
    operation_counts = {
        "create_part": 0,
        "kill_part": 0,
        "part_change_type": 0,
        "move": 0,
        "swap": 0,
        "pressure_write": 0,
        "air_velocity_write": 0,
        "air_heat_write": 0,
        "gravity_write": 0,
        "electricity_write": 0,
        "rng": 0,
    }
    root_operation_counts = dict.fromkeys(operation_counts, 0)
    for element in resolved_elements:
        observations = element["observations"]
        risk_counts[observations["gpu_lexical_risk"]] += 1
        for key in operation_counts:
            operation_counts[key] += int(bool(observations[key]))
    for root_record in root_list:
        observations = compact_observations(root_record["analysis"])
        root_risk_counts[observations["gpu_lexical_risk"]] += 1
        for key in root_operation_counts:
            root_operation_counts[key] += int(bool(observations[key]))

    active_source_paths = [root / item[1] for item in parsed]
    implementation_source_paths = [
        root / function["source"]
        for item in root_list
        for function in item["reachable_functions"]
    ]
    manifest_hash, manifest = input_manifest(
        root, [registry_path, *active_source_paths, *implementation_source_paths]
    )
    inventory = {
        "schema": "tpt-zh-omnipack.element-update-inventory",
        "schema_version": SCHEMA_VERSION,
        "generator": {"path": GENERATOR_PATH, "version": GENERATOR_VERSION},
        "base_commit": baseline_commit,
        "analysis_complete": False,
        "analysis_method": {
            "kind": "deterministic conservative lexical scan",
            "registry_source": "src/simulation/elements/meson.build",
            "binding_resolution": (
                "constructor Update assignments plus reachable configurator assignments"
            ),
            "implementation_scope": (
                "resolved update root plus same-file callees and globally unique bare-function "
                "callees; member calls, ambiguous overloads, headers, macros, and function "
                "pointers are not followed"
            ),
            "shared_update_semantics": (
                "each shared root is a union of all lexically reached paths; per-element results "
                "can over-report branches for other element types"
            ),
            "absence_semantics": (
                "false/not_detected means no supported lexical pattern matched; it does not "
                "prove semantic absence"
            ),
            "ast_complete": False,
            "data_flow_complete": False,
            "alias_analysis_complete": False,
            "inter_translation_unit_call_graph_complete": False,
            "preprocessor_evaluation_complete": False,
            "runtime_lua_overrides_in_scope": False,
            "text_hash_normalization": "CRLF is normalized to LF",
        },
        "input_manifest_sha256": manifest_hash,
        "input_manifest": manifest,
        "summary": {
            "registered_elements": len(element_records),
            "custom_update_bindings": len(custom_elements),
            "direct_update_bindings": direct_bindings,
            "configurator_update_bindings": configurator_bindings,
            "elements_without_custom_update": len(no_update_elements),
            "resolved_update_bindings": len(resolved_elements),
            "unresolved_update_bindings": len(unresolved),
            "unique_implementation_roots": len(root_list),
            "source_local_update_roots": sum(
                1 for item in root_list if item["symbol"] == "update"
            ),
            "shared_named_update_roots": sum(
                1 for item in root_list if item["symbol"] != "update"
            ),
            "upstream_elements": sum(
                1 for item in element_records if item["origin"] == "upstream"
            ),
            "omnipack_elements": sum(
                1 for item in element_records if item["origin"] == "omnipack"
            ),
            "upstream_custom_update_bindings": sum(
                1
                for item in custom_elements
                if item["origin"] == "upstream"
            ),
            "omnipack_custom_update_bindings": sum(
                1
                for item in custom_elements
                if item["origin"] == "omnipack"
            ),
            "classification_counts": {"UNKNOWN": len(element_records)},
            "gpu_lexical_risk_counts": risk_counts,
            "gpu_lexical_risk_root_counts": root_risk_counts,
            "elements_with_detected_operations": operation_counts,
            "implementation_roots_with_detected_operations": root_operation_counts,
            "input_manifest_file_count": len(manifest),
        },
        "unresolved_bindings": unresolved,
        "implementation_roots": root_list,
        "elements": element_records,
    }
    validate_inventory(inventory)
    return inventory


def validate_inventory(inventory: dict[str, object]) -> None:
    if inventory.get("analysis_complete") is not False:
        raise ValueError("analysis_complete must remain false for lexical inventory")
    summary = inventory["summary"]
    elements = inventory["elements"]
    roots = inventory["implementation_roots"]
    if summary["registered_elements"] != len(elements):
        raise ValueError("registered element count mismatch")
    ids = [item["id"] for item in elements]
    if ids != sorted(ids) or len(ids) != len(set(ids)):
        raise ValueError("element IDs are not unique and sorted")
    if any(item["classification"] != "UNKNOWN" for item in elements):
        raise ValueError("lexical inventory must leave every classification UNKNOWN")
    root_ids = [item["root_id"] for item in roots]
    if root_ids != sorted(root_ids) or len(root_ids) != len(set(root_ids)):
        raise ValueError("implementation roots are not unique and sorted")
    root_id_set = set(root_ids)
    for element in elements:
        root_id = element["implementation_root_id"]
        if root_id is not None and root_id not in root_id_set:
            raise ValueError(f"unknown implementation root reference: {root_id}")
    if summary["unresolved_update_bindings"] != len(inventory["unresolved_bindings"]):
        raise ValueError("unresolved binding count mismatch")
    if summary["custom_update_bindings"] != (
        summary["resolved_update_bindings"] + summary["unresolved_update_bindings"]
    ):
        raise ValueError("custom binding accounting mismatch")


def canonical_json(inventory: dict[str, object]) -> bytes:
    return (json.dumps(inventory, ensure_ascii=False, indent=2) + "\n").encode("utf-8")


def canonical_lf(payload: bytes) -> bytes:
    return payload.replace(b"\r\n", b"\n")


def run_self_tests() -> None:
    sample = """// comment {\nstatic int update(UPDATE_FUNC_ARGS)\n{\n  parts[i].temp += 1;\n  parts[ID(r)].life = 4;\n  sim->pv[y/CELL][x/CELL] += 0.5f;\n  sim->create_part(-1, x, y, PT_FIRE);\n  return 0;\n}\n"""
    masked = mask_cpp(sample)
    assert masked.count("\n") == sample.count("\n")
    functions = extract_functions("sample.cpp", sample)
    assert len(functions) == 1 and functions[0].short_name == "update"
    analysis = analyze_implementation(functions)
    compact = compact_observations(analysis)
    assert compact["self_reads"] == ["temp"]
    assert compact["self_writes"] == ["temp"]
    assert compact["non_self_writes"] == ["life"]
    assert compact["pressure_write"] is True
    assert compact["create_part"] is True
    assert compact["gpu_lexical_risk"] == "high"
    literal_sample = 'auto s = "parts[i].temp = 0"; /* sim->kill_part(i); */\n'
    assert "parts" not in mask_cpp(literal_sample)
    assert "kill_part" not in mask_cpp(literal_sample)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repository root (default: inferred from this script)",
    )
    parser.add_argument(
        "--base-commit",
        required=True,
        help="40-hex committed baseline audited by this inventory",
    )
    output_group = parser.add_mutually_exclusive_group()
    output_group.add_argument("--output", type=Path, help="write canonical JSON")
    output_group.add_argument("--check", type=Path, help="compare canonical JSON with file")
    parser.add_argument(
        "--self-test", action="store_true", help="run scanner unit self-tests before inventory"
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv if argv is not None else sys.argv[1:])
    if not re.fullmatch(r"[0-9a-fA-F]{40}", args.base_commit):
        print("error: --base-commit must be exactly 40 hexadecimal characters", file=sys.stderr)
        return 2
    try:
        if args.self_test:
            run_self_tests()
        root = args.root.resolve()
        inventory = build_inventory(root, args.base_commit.lower())
        payload = canonical_json(inventory)
        # A JSON round trip is part of every invocation's structural self-check.
        decoded = json.loads(payload)
        validate_inventory(decoded)
        if args.check:
            expected = args.check.resolve().read_bytes()
            normalized_expected = canonical_lf(expected)
            if normalized_expected != payload:
                print(f"FAIL: inventory drift: {args.check}", file=sys.stderr)
                return 1
            action = (
                f"checked={args.check} "
                f"line_ending_normalized={str(expected != normalized_expected).lower()}"
            )
        elif args.output:
            destination = args.output.resolve()
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(payload)
            action = f"wrote={args.output}"
        else:
            sys.stdout.buffer.write(payload)
            action = "wrote=stdout"
        summary = inventory["summary"]
        if args.output or args.check:
            print(
                "PASS: "
                f"{action} registered={summary['registered_elements']} "
                f"bindings={summary['custom_update_bindings']} "
                f"roots={summary['unique_implementation_roots']} "
                f"unresolved={summary['unresolved_update_bindings']} "
                f"sha256={hashlib.sha256(payload).hexdigest()}"
            )
        return 0
    except (OSError, UnicodeError, ValueError, AssertionError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
