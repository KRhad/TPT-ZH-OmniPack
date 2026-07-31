#!/usr/bin/env python3
"""Extract Lua-defined TPT elements, callbacks and global-risk markers."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
from typing import Sequence

from catalog_common import read_json, read_text, source_files, write_json


ALLOCATE = re.compile(
    r"(?:(?:local\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*=\s*)?"
    r"(?:elements|elem)\.allocate\s*\(\s*([^,]+),\s*[\"']([^\"']+)[\"']\s*\)"
)
PROPERTY = re.compile(
    r"(?:elements|elem)\.property\s*\(\s*([^,]+),\s*[\"']([^\"']+)[\"']\s*,\s*([^\r\n\)]+)"
)
ELEMENT_TABLE = re.compile(r"(?:elements|elem)\.element\s*\(\s*([^,]+),\s*\{(.*?)\}\s*\)", re.S)
PT_REF = re.compile(r"(?:(?:elements|elem)\.)?(?:DEFAULT_|[A-Z0-9]+_)?PT_([A-Z0-9_]+)")


def normalize_expression(value: str) -> str:
    return " ".join(value.strip().rstrip(",").split())[:500]


def risk_markers(text: str) -> dict[str, object]:
    lowered = text.lower()
    global_callbacks = sorted({
        marker for marker in (
            "tpt.register_step", "tpt.register_keypress", "tpt.register_mouseclick",
            "event.register(event.tick", "event.register(event.keypress",
            "event.register(event.beforedraw", "event.register(event.afterdraw",
        ) if marker in lowered
    })
    global_scan = bool(
        re.search(r"for\s+\w+\s*=\s*0\s*,\s*(?:sim\.)?NPART", text, re.I)
        or re.search(r"for\s+\w+\s+in\s+(?:sim\.)?parts", text, re.I)
    )
    graphics_simulation = bool(
        re.search(r"(?:graphics|gfx)[^\n]{0,120}(?:create|kill|partProperty|parts\[)", text, re.I | re.S)
    )
    unbounded_loop = bool(re.search(r"\bwhile\s+true\s+do\b|\brepeat\b(?:(?!until).){1000,}", text, re.I | re.S))
    return {
        "global_callbacks": global_callbacks,
        "global_scan": global_scan,
        "graphics_simulation": graphics_simulation,
        "unbounded_loop": unbounded_loop,
    }


def extract_file(repo_id: str, repo: Path, path: Path, commit: str, license_status: str) -> list[dict[str, object]]:
    text = read_text(path)
    risks = risk_markers(text)
    allocations = list(ALLOCATE.finditer(text))
    properties = list(PROPERTY.finditer(text))
    records: list[dict[str, object]] = []

    for ordinal, allocation in enumerate(allocations):
        variable = allocation.group(1) or f"allocation_{ordinal}"
        namespace = normalize_expression(allocation.group(2))
        code = allocation.group(3)
        values: dict[str, str] = {}
        callbacks: dict[str, str] = {"Update": "", "Graphics": "", "Create": ""}
        for prop in properties:
            target = normalize_expression(prop.group(1))
            if target not in {variable, code, f'"{code}"', f"'{code}'"} and variable not in target:
                continue
            key = prop.group(2)
            value = normalize_expression(prop.group(3))
            values[key] = value
            if key in callbacks:
                callbacks[key] = value

        for table in ELEMENT_TABLE.finditer(text):
            target = normalize_expression(table.group(1))
            if variable not in target and code not in target:
                continue
            for entry in re.finditer(r"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^,\r\n}]+)", table.group(2)):
                values.setdefault(entry.group(1), normalize_expression(entry.group(2)))

        dependencies = sorted({f"PT_{name}" for name in PT_REF.findall(text) if name != code})
        create_count = len(re.findall(r"(?:sim\.)?(?:partCreate|create_part|createPart)\s*\(", text))
        kill_count = len(re.findall(r"(?:sim\.)?(?:partKill|kill_part|killPart)\s*\(", text))
        risk_level = "high" if any((risks["global_scan"], risks["graphics_simulation"], risks["unbounded_loop"])) else (
            "medium" if risks["global_callbacks"] or create_count else "low"
        )
        name = values.get("Name", values.get("name", code)).strip("\"'")
        identifier = f"{namespace.strip(chr(34) + chr(39))}_PT_{code}" if namespace else code
        records.append({
            "source_mod": repo_id,
            "source_commit": commit,
            "source_file": path.relative_to(repo).as_posix(),
            "source_identifier": identifier,
            "source_id": variable,
            "source_name": name,
            "source_code": code,
            "category": values.get("MenuSection", values.get("menu", "")),
            "state": values.get("State", values.get("state", "unknown")).strip("\"'"),
            "color": values.get("Colour", values.get("Color", values.get("colour", ""))),
            "properties": values,
            "properties_summary": values.get("Properties", values.get("properties", "")),
            "update_function": callbacks["Update"],
            "graphics_function": callbacks["Graphics"],
            "create_function": callbacks["Create"],
            "dependencies": dependencies,
            "reaction_count": 0,
            "reaction_targets": [],
            "global_scan": risks["global_scan"],
            "global_callbacks": risks["global_callbacks"],
            "graphics_simulation": risks["graphics_simulation"],
            "unbounded_loop": risks["unbounded_loop"],
            "particle_creation": create_count,
            "particle_deletion": kill_count,
            "performance_risk": risk_level,
            "save_risk": "review" if re.search(r"\b(?:ctype|tmp2?|life)\b", text) else "low",
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
    scan_data = read_json(root / "external/metadata/repository_scan.json", [])
    scan_rows = {row["mod_id"]: row for row in scan_data}
    repo_ids = args.repository or sorted(path.name for path in repositories.iterdir() if (path / ".git").exists())
    records: list[dict[str, object]] = []
    for repo_id in repo_ids:
        repo = repositories / repo_id
        if not (repo / ".git").exists():
            continue
        metadata = scan_rows.get(repo_id, {})
        commit = str(metadata.get("source_commit", ""))
        license_status = "verified" if metadata.get("license_verified") == "true" else "unknown"
        for path in source_files(repo, {".lua"}):
            records.extend(extract_file(repo_id, repo, path, commit, license_status))
    raw_count = 0
    for metadata in scan_data:
        if metadata.get("audit_status") != "downloaded_source":
            continue
        repo_id = str(metadata["mod_id"])
        if args.repository and repo_id not in args.repository:
            continue
        repo = Path(str(metadata.get("path", "")))
        if not repo.is_absolute():
            repo = root / repo
        path = repo / str(metadata.get("raw_source_file", ""))
        if not path.is_file():
            continue
        records.extend(
            extract_file(
                repo_id,
                repo,
                path,
                str(metadata.get("source_commit", "")),
                "verified" if metadata.get("license_verified") == "true" else "unknown",
            )
        )
        raw_count += 1
    records.sort(key=lambda row: (str(row["source_mod"]), str(row["source_file"]), str(row["source_code"])))
    output = args.output or root / "external/metadata/lua_elements.json"
    write_json(output, records)
    print(
        f"extract-lua-elements: PASS repositories={len(repo_ids)} "
        f"raw_sources={raw_count} elements={len(records)} output={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
