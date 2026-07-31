#!/usr/bin/env python3
"""Structurally extract C/C++ TPT element constructors and callbacks."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
from typing import Sequence

from catalog_common import balanced_block, read_json, read_text, source_files, write_json


CONSTRUCTOR = re.compile(
    r"(?<!~)(?:Element::Element_|Element_)([A-Z0-9_]+)(?:::\1)?\s*\([^)]*\)\s*(?::[^\{]+)?\{"
)
STRING_ASSIGN = re.compile(r"\b([A-Za-z][A-Za-z0-9_]*)\s*=\s*[\"']([^\"']*)[\"']\s*;")
VALUE_ASSIGN = re.compile(r"\b([A-Za-z][A-Za-z0-9_]*)\s*=\s*([^;\r\n]+)\s*;")
PT_REF = re.compile(r"\bPT_([A-Z0-9_]+)\b")
CALLBACK = re.compile(r"\b(?:Element_)?([A-Z0-9_]+)::(update|graphics|create)\s*\(")

PROPERTY_FIELDS = {
    "Identifier", "Name", "Colour", "Color", "MenuVisible", "MenuSection",
    "Enabled", "Advection", "AirDrag", "AirLoss", "Loss", "Collision",
    "Gravity", "Diffusion", "HotAir", "Falldown", "Weight", "HeatConduct",
    "HeatCapacity", "LowPressure", "LowPressureTransition", "HighPressure",
    "HighPressureTransition", "LowTemperature", "LowTemperatureTransition",
    "HighTemperature", "HighTemperatureTransition", "Properties", "State",
    "Flammable", "Explosive", "Meltable", "Hardness", "PhotonReflectWavelengths",
}


def assignment_map(block: str) -> dict[str, str]:
    values = {match.group(1): match.group(2) for match in STRING_ASSIGN.finditer(block)}
    for match in VALUE_ASSIGN.finditer(block):
        if match.group(1) in PROPERTY_FIELDS and match.group(1) not in values:
            values[match.group(1)] = " ".join(match.group(2).split())
    return values


def derive_state(properties: str, explicit: str) -> str:
    value = explicit or properties
    for token, state in (
        ("TYPE_GAS", "gas"), ("TYPE_LIQUID", "liquid"),
        ("TYPE_PART", "powder"), ("TYPE_SOLID", "solid"), ("TYPE_ENERGY", "energy"),
    ):
        if token in value:
            return state
    return "unknown"


def callback_names(text: str, code: str) -> dict[str, str]:
    result = {"update": "", "graphics": "", "create": ""}
    for match in CALLBACK.finditer(text):
        if match.group(1) == code:
            result[match.group(2)] = f"{match.group(1)}::{match.group(2)}"
    for kind, marker in (("update", "Update = &"), ("graphics", "Graphics = &"), ("create", "Create = &")):
        if not result[kind]:
            match = re.search(re.escape(marker) + r"([A-Za-z0-9_:]+)", text)
            if match:
                result[kind] = match.group(1)
    return result


def extract_file(repo_id: str, repo: Path, path: Path, commit: str, license_status: str) -> list[dict[str, object]]:
    text = read_text(path)
    records: list[dict[str, object]] = []
    constructors = list(CONSTRUCTOR.finditer(text))
    destructor_only = bool(re.search(r"Element_[A-Z0-9_]+::~Element_[A-Z0-9_]+\s*\(", text))
    if (
        not constructors
        and not destructor_only
        and path.parent.name.lower() == "elements"
        and path.suffix.lower() in {".cpp", ".c", ".cc"}
    ):
        constructors = []
        synthetic_code = path.stem.upper().removeprefix("ELEMENT_")
        if re.fullmatch(r"[A-Z0-9_]+", synthetic_code) and any(
            marker in text for marker in ("Identifier", "MenuSection", "TYPE_", "UPDATE_FUNC_ARGS")
        ):
            block = text
            constructors = [(synthetic_code, block)]  # type: ignore[assignment]

    iterable: list[tuple[str, str]] = []
    for constructor in constructors:
        if isinstance(constructor, tuple):
            iterable.append(constructor)
        else:
            opening = text.find("{", constructor.start())
            iterable.append((constructor.group(1), balanced_block(text, opening)))

    for code, block in iterable:
        values = assignment_map(block)
        callbacks = callback_names(text, code)
        dependencies = sorted({f"PT_{name}" for name in PT_REF.findall(text) if name != code})
        identifier = values.get("Identifier", "")
        name = values.get("Name", code)
        properties = values.get("Properties", "")
        transition_fields = {
            key: values.get(key, "")
            for key in (
                "LowPressure", "LowPressureTransition", "HighPressure", "HighPressureTransition",
                "LowTemperature", "LowTemperatureTransition", "HighTemperature", "HighTemperatureTransition",
            )
        }
        global_scan = bool(re.search(r"for\s*\([^;]+;[^;]*(?:NPART|parts\.active)", text))
        neighbor_scan = bool(re.search(r"(?:rx|nx)\s*=\s*-?\d|pmap\[|photons\[", text))
        create_calls = len(re.findall(r"\b(?:create_part|CreatePart)\s*\(", text))
        kill_calls = len(re.findall(r"\b(?:kill_part|KillPart)\s*\(", text))
        records.append({
            "source_mod": repo_id,
            "source_commit": commit,
            "source_file": path.relative_to(repo).as_posix(),
            "source_identifier": identifier,
            "source_id": "",
            "source_name": name,
            "source_code": code,
            "category": values.get("MenuSection", ""),
            "state": derive_state(properties, values.get("State", "")),
            "color": values.get("Colour", values.get("Color", "")),
            "properties": {field: values[field] for field in sorted(values) if field in PROPERTY_FIELDS},
            "properties_summary": properties,
            "transitions": transition_fields,
            "update_function": callbacks["update"],
            "graphics_function": callbacks["graphics"],
            "create_function": callbacks["create"],
            "dependencies": dependencies,
            "reaction_count": 0,
            "reaction_targets": [],
            "neighbor_scan": neighbor_scan,
            "global_scan": global_scan,
            "particle_creation": create_calls,
            "particle_deletion": kill_calls,
            "performance_risk": "high" if global_scan or create_calls > 8 else "medium" if create_calls or neighbor_scan else "low",
            "save_risk": "review" if any(token in text for token in ("ctype", "tmp", "tmp2")) else "low",
            "license_status": license_status,
            "port_decision": "unreviewed",
        })
    return records


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--repository", action="append", default=[])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    repositories = root / "external/repositories"
    scan_rows = {row["mod_id"]: row for row in read_json(root / "external/metadata/repository_scan.json", [])}
    repo_ids = args.repository or sorted(path.name for path in repositories.iterdir() if (path / ".git").exists())
    records: list[dict[str, object]] = []
    for repo_id in repo_ids:
        repo = repositories / repo_id
        if not (repo / ".git").exists():
            continue
        metadata = scan_rows.get(repo_id, {})
        commit = str(metadata.get("source_commit", ""))
        license_status = "verified" if metadata.get("license_verified") == "true" else "unknown"
        for path in source_files(repo, {".cpp", ".cc", ".c"}):
            records.extend(extract_file(repo_id, repo, path, commit, license_status))
    records.sort(key=lambda row: (str(row["source_mod"]), str(row["source_file"]), str(row["source_code"])))
    output = args.output or root / "external/metadata/cpp_elements.json"
    write_json(output, records)
    print(f"extract-cpp-elements: PASS repositories={len(repo_ids)} elements={len(records)} output={output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
