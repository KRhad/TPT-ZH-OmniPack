#!/usr/bin/env python3
"""Normalize raw C++ and Lua element extraction into one candidate model."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
from typing import Any, Sequence

from catalog_common import read_json, write_json


STATE_TOKENS = {
    "TYPE_SOLID": "solid", "ST_SOLID": "solid",
    "TYPE_PART": "powder", "ST_NONE": "powder",
    "TYPE_LIQUID": "liquid", "ST_LIQUID": "liquid",
    "TYPE_GAS": "gas", "ST_GAS": "gas",
    "TYPE_ENERGY": "energy", "ST_ENERGY": "energy",
}


def clean(value: Any) -> str:
    if value is None:
        return ""
    return " ".join(str(value).replace("\x00", "").split())[:1000]


def normalized_state(record: dict[str, Any]) -> str:
    explicit = clean(record.get("state", "")).lower()
    if explicit in {"solid", "powder", "liquid", "gas", "energy"}:
        return explicit
    summary = clean(record.get("properties_summary", "")) + " " + clean(record.get("properties", ""))
    for token, state in STATE_TOKENS.items():
        if token in summary:
            return state
    return "unknown"


def normalized_identifier(record: dict[str, Any]) -> str:
    value = clean(record.get("source_identifier", "")).strip("\"'")
    if re.fullmatch(r"[A-Za-z][A-Za-z0-9_]{2,127}", value):
        return value.upper()
    code = clean(record.get("source_code", "")).upper()
    mod = re.sub(r"[^A-Z0-9]+", "_", clean(record.get("source_mod", "")).upper()).strip("_")
    return f"{mod}_PT_{code}" if code else ""


def normalize(record: dict[str, Any]) -> dict[str, Any]:
    properties = record.get("properties", {})
    if not isinstance(properties, dict):
        properties = {"raw": clean(properties)}
    flags = sorted(set(re.findall(r"\b(?:TYPE|PROP|STATE)_[A-Z0-9_]+\b", clean(properties))))
    return {
        **record,
        "source_identifier": normalized_identifier(record),
        "source_name": clean(record.get("source_name", record.get("source_code", ""))).strip("\"'"),
        "source_code": clean(record.get("source_code", "")).upper(),
        "category": clean(record.get("category", "")),
        "state": normalized_state(record),
        "color": clean(record.get("color", "")),
        "properties": {str(key): clean(value) for key, value in properties.items()},
        "property_flags": flags,
        "dependencies": sorted({clean(value) for value in record.get("dependencies", []) if clean(value)}),
        "global_scan": bool(record.get("global_scan", False)),
        "particle_creation": int(record.get("particle_creation", 0) or 0),
        "particle_deletion": int(record.get("particle_deletion", 0) or 0),
        "performance_risk": clean(record.get("performance_risk", "unknown")),
        "save_risk": clean(record.get("save_risk", "unknown")),
        "license_status": clean(record.get("license_status", "unknown")),
        "port_decision": clean(record.get("port_decision", "unreviewed")),
    }


def record_quality(record: dict[str, Any]) -> tuple[int, int, int, int]:
    source_file = str(record.get("source_file", "")).lower()
    preferred_path = int("src/simulation/elements/" in source_file or "src/elements/" in source_file)
    explicit_identifier = int(str(record.get("source_identifier", "")).startswith(("DEFAULT_", "MOD_", "OMNI_")))
    callbacks = sum(bool(record.get(field)) for field in ("update_function", "graphics_function", "create_function"))
    property_count = len(record.get("properties", {})) if isinstance(record.get("properties"), dict) else 0
    return preferred_path, explicit_identifier, callbacks, property_count


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--cpp", type=Path)
    parser.add_argument("--lua", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    cpp = read_json(args.cpp or root / "external/metadata/cpp_elements.json", [])
    lua = read_json(args.lua or root / "external/metadata/lua_elements.json", [])
    records = [normalize(record) for record in [*cpp, *lua]]
    unique: dict[tuple[str, str], dict[str, Any]] = {}
    for record in records:
        key = (record["source_mod"], record["source_identifier"])
        existing = unique.get(key)
        if existing is None or record_quality(record) > record_quality(existing):
            if existing is not None:
                record["alternative_source_files"] = sorted({
                    *existing.get("alternative_source_files", []), existing["source_file"],
                    *record.get("alternative_source_files", []),
                })
            unique[key] = record
        elif existing is not None:
            existing["alternative_source_files"] = sorted({
                *existing.get("alternative_source_files", []), record["source_file"],
            })
    output_records = sorted(unique.values(), key=lambda row: (row["source_mod"], row["source_identifier"], row["source_file"]))
    output = args.output or root / "external/metadata/candidate_elements.json"
    write_json(output, output_records)
    print(
        f"extract-element-properties: PASS cpp={len(cpp)} lua={len(lua)} "
        f"unique={len(output_records)} output={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
