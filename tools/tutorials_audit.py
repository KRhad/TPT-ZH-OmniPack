#!/usr/bin/env python3
"""Fail-closed audit for the 0.2 example OPS and tutorial contracts."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
SPEC_PATH = ROOT / "examples" / "0.2.0" / "example-spec.json"
TUTORIAL_PATH = ROOT / "docs" / "TUTORIALS_0.2.json"
EXAMPLE_DIR = ROOT / "examples" / "0.2.0"
STAMP_ID_RE = re.compile(r"^[0-9A-Fa-f]{10}$")
ELEMENTS = {
    "PERO", "PATH", "HUMS", "FERT", "WATR", "CATA", "SLAG", "ACID",
    "FLUX", "SSIL", "LEAD", "NCRM", "LAVA", "SPRK", "NWST", "POLY",
    "RSHD", "NUTR",
}
REQUIRED_TUTORIAL_FIELDS = {
    "id", "example_id", "title_zh", "title_en", "goal_zh", "goal_en",
    "hint_zh", "hint_en", "success", "failure", "result_zh", "result_en",
    "next",
}


def read_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain an object")
    return value


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def audit_spec(spec: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if spec.get("schema_version") != 1:
        errors.append("example spec schema_version must be 1")
    if spec.get("content_version") != "0.2.0-dev":
        errors.append("example spec content_version must be 0.2.0-dev")
    examples = spec.get("examples")
    if not isinstance(examples, list) or len(examples) != 7:
        return errors + ["example spec must contain exactly 7 examples"]
    ids: set[str] = set()
    filenames: set[str] = set()
    for index, example in enumerate(examples):
        if not isinstance(example, dict):
            errors.append(f"example {index} is not an object")
            continue
        identifier = example.get("id")
        filename = example.get("filename")
        if not isinstance(identifier, str) or not re.fullmatch(r"[a-z0-9-]+", identifier):
            errors.append(f"example {index} has invalid id")
        elif identifier in ids:
            errors.append(f"duplicate example id: {identifier}")
        else:
            ids.add(identifier)
        if not isinstance(filename, str) or Path(filename).name != filename or not filename.endswith(".stm"):
            errors.append(f"example {identifier} has unsafe filename")
        elif filename in filenames:
            errors.append(f"duplicate example filename: {filename}")
        else:
            filenames.add(filename)
        if not isinstance(example.get("purpose"), str) or not example["purpose"].strip():
            errors.append(f"example {identifier} lacks purpose")
        particles = example.get("particles")
        if not isinstance(particles, list) or not particles:
            errors.append(f"example {identifier} lacks particles")
            continue
        positions: set[tuple[int, int]] = set()
        for particle in particles:
            if not isinstance(particle, dict):
                errors.append(f"example {identifier} has non-object particle")
                continue
            element = particle.get("element")
            if element not in ELEMENTS:
                errors.append(f"example {identifier} has unsupported element {element}")
            try:
                position = (int(particle["x"]), int(particle["y"]))
                float(particle["temp"])
            except (KeyError, TypeError, ValueError):
                errors.append(f"example {identifier} has invalid particle coordinates/temp")
                continue
            if position in positions:
                errors.append(f"example {identifier} has duplicate particle position {position}")
            positions.add(position)
            if "ctype" in particle and particle["ctype"] not in ELEMENTS:
                errors.append(f"example {identifier} has unsupported ctype {particle['ctype']}")
    return errors


def audit_tutorials(tutorials: dict[str, Any], example_ids: set[str]) -> list[str]:
    errors: list[str] = []
    if tutorials.get("schema_version") != 1:
        errors.append("tutorial schema_version must be 1")
    if tutorials.get("content_version") != "0.2.0-dev":
        errors.append("tutorial content_version must be 0.2.0-dev")
    rows = tutorials.get("tutorials")
    if not isinstance(rows, list) or len(rows) != 8:
        return errors + ["tutorial catalog must contain exactly 8 entries"]
    ids: set[str] = set()
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            errors.append(f"tutorial {index} is not an object")
            continue
        missing = sorted(REQUIRED_TUTORIAL_FIELDS - row.keys())
        if missing:
            errors.append(f"tutorial {index} missing fields: {', '.join(missing)}")
        identifier = row.get("id")
        if not isinstance(identifier, str) or identifier in ids:
            errors.append(f"tutorial {index} has duplicate/invalid id")
        else:
            ids.add(identifier)
        if row.get("example_id") not in example_ids:
            errors.append(f"tutorial {identifier} references missing example")
        for field in REQUIRED_TUTORIAL_FIELDS - {"example_id", "id"}:
            if field in row and (not isinstance(row[field], str) or not row[field].strip()):
                errors.append(f"tutorial {identifier} field {field} is empty")
    next_values = {row.get("next") for row in rows if isinstance(row, dict)}
    if "END" not in next_values:
        errors.append("tutorial chain must terminate at END")
    return errors


def audit_generated_manifest(manifest: dict[str, Any], spec: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if manifest.get("schema_version") != 1:
        errors.append("generated manifest schema_version must be 1")
    if not re.fullmatch(r"[0-9a-f]{40}", str(manifest.get("source_commit", ""))):
        errors.append("generated manifest source_commit must be a full lowercase revision")
    if not re.fullmatch(r"[0-9A-F]{64}", str(manifest.get("generator_exe_sha256", ""))):
        errors.append("generated manifest generator_exe_sha256 must be SHA-256")
    rows = manifest.get("examples")
    if not isinstance(rows, list) or len(rows) != 7:
        errors.append("generated manifest must contain exactly 7 examples")
        return errors
    expected = {row["id"]: row for row in spec["examples"]}
    seen_stamps: set[str] = set()
    for row in rows:
        if not isinstance(row, dict) or row.get("id") not in expected:
            errors.append("generated manifest contains an unknown example")
            continue
        if not STAMP_ID_RE.fullmatch(str(row.get("stamp_id", ""))):
            errors.append(f"invalid stamp_id for {row.get('id')}")
        elif row["stamp_id"] in seen_stamps:
            errors.append(f"duplicate stamp_id for {row.get('id')}")
        else:
            seen_stamps.add(row["stamp_id"])
        if row.get("filename") != expected[row["id"]]["filename"]:
            errors.append(f"filename drift for {row['id']}")
        if not re.fullmatch(r"[0-9A-F]{64}", str(row.get("sha256", ""))):
            errors.append(f"invalid SHA-256 for {row.get('id')}")
        path = EXAMPLE_DIR / str(row.get("filename", ""))
        if not path.is_file():
            errors.append(f"missing generated example file: {path.name}")
        elif sha256(path) != row["sha256"]:
            errors.append(f"generated example hash mismatch: {path.name}")
    return errors


def audit_repository(require_generated: bool = False) -> list[str]:
    spec = read_json(SPEC_PATH)
    tutorials = read_json(TUTORIAL_PATH)
    errors = audit_spec(spec)
    example_ids = {row.get("id") for row in spec.get("examples", []) if isinstance(row, dict)}
    errors.extend(audit_tutorials(tutorials, example_ids))
    manifest_path = EXAMPLE_DIR / "manifest.json"
    if require_generated or manifest_path.exists():
        if not manifest_path.is_file():
            errors.append("generated example manifest is missing")
        else:
            errors.extend(audit_generated_manifest(read_json(manifest_path), spec))
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-generated", action="store_true")
    args = parser.parse_args()
    errors = audit_repository(require_generated=args.require_generated)
    if errors:
        for error in errors:
            print(f"tutorials-audit: ERROR {error}")
        return 1
    print("tutorials-audit: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
