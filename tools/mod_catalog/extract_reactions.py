#!/usr/bin/env python3
"""Extract reaction targets and simulation-operation risk from candidate sources."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
from typing import Sequence

from catalog_common import read_json, read_text, write_json


PT_REF = re.compile(r"\bPT_([A-Z0-9_]+)\b")
OPERATIONS = {
    "create_part": re.compile(r"\b(?:create_part|CreatePart|partCreate)\s*\("),
    "kill_part": re.compile(r"\b(?:kill_part|KillPart|partKill)\s*\("),
    "change_type": re.compile(r"\b(?:part_change_type|ChangeType|partChangeType)\s*\("),
    "direct_type_write": re.compile(r"(?:parts\s*\[[^\]]+\]|\w+)\.type\s*="),
    "ctype": re.compile(r"\.ctype\b"),
    "tmp": re.compile(r"\.tmp\b"),
    "tmp2": re.compile(r"\.tmp2\b"),
    "temperature": re.compile(r"\.temp\b|\btemperature\b", re.I),
    "pressure": re.compile(r"\bpv\s*\[|\bpressure\b|\.pressure\b", re.I),
    "electricity": re.compile(r"\bPT_SPRK\b|\.life\s*[<>=].*SPRK", re.I),
    "neutron": re.compile(r"\bPT_NEUT\b|\bneutron", re.I),
}


def probability_markers(text: str) -> list[str]:
    markers: set[str] = set()
    for pattern in (
        r"rng\.chance\s*\(([^)]+)\)",
        r"rand\s*\(\s*\)\s*%\s*([0-9]+)",
        r"math\.random\s*\(([^)]+)\)",
    ):
        for match in re.finditer(pattern, text):
            markers.add(" ".join(match.group(1).split())[:120])
    return sorted(markers)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--elements", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    repositories = root / "external/repositories"
    elements = read_json(args.elements or root / "external/metadata/candidate_elements.json", [])
    source_roots: dict[str, Path] = {}
    for row in read_json(root / "external/metadata/repository_scan.json", []):
        if not row.get("path"):
            continue
        source_root = Path(str(row["path"]))
        if not source_root.is_absolute():
            source_root = root / source_root
        source_roots[str(row["mod_id"])] = source_root
    records: list[dict[str, object]] = []
    file_cache: dict[Path, str] = {}
    for element in elements:
        source_root = source_roots.get(str(element["source_mod"]), repositories / element["source_mod"])
        path = source_root / element["source_file"]
        if not path.is_file():
            continue
        text = file_cache.setdefault(path, read_text(path))
        self_code = str(element.get("source_code", ""))
        targets = sorted({f"PT_{name}" for name in PT_REF.findall(text) if name != self_code})
        operations = {name: len(pattern.findall(text)) for name, pattern in OPERATIONS.items()}
        neighbor_scan = bool(
            re.search(r"for\s*\([^;]*(?:rx|nx|x)[^;]*;[^;]*(?:rx|nx|x)", text)
            or re.search(r"\bpmap\s*\[|\bphotons\s*\[", text)
        )
        global_scan = bool(re.search(r"for\s*\([^;]+;[^;]*(?:NPART|parts\.active)", text))
        reaction_count = len(targets) + sum(
            operations[name] for name in ("create_part", "kill_part", "change_type", "direct_type_write")
        )
        records.append({
            "source_mod": element["source_mod"],
            "source_commit": element.get("source_commit", ""),
            "source_file": element["source_file"],
            "source_identifier": element.get("source_identifier", ""),
            "reaction_count": reaction_count,
            "reaction_targets": targets,
            "operations": operations,
            "probability_markers": probability_markers(text),
            "neighbor_scan": neighbor_scan,
            "global_scan": global_scan,
            "event_budget_detected": bool(re.search(r"(?:event|reaction|create).*budget|budget.*(?:event|reaction|create)", text, re.I)),
            "risk": "high" if global_scan or operations["create_part"] > 16 else "medium" if reaction_count else "low",
        })
    output = args.output or root / "external/metadata/reactions.json"
    write_json(output, records)
    print(
        f"extract-reactions: PASS elements={len(records)} "
        f"targets={sum(len(row['reaction_targets']) for row in records)} output={output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
