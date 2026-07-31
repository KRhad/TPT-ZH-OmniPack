#!/usr/bin/env python3
"""Compare candidate core files with a pinned official TPT checkout."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Sequence

from catalog_common import read_json, sha256, source_files, write_json


def category(relative: str) -> str:
    normalized = "/" + relative.replace("\\", "/").lower()
    if any(token in normalized for token in ("gamesave", "savefile", "saverenderer", "/save/")):
        return "save"
    if any(token in normalized for token in ("/http/", "requestmanager", "uploadsave", "getsave")):
        return "network"
    if "/gui/" in normalized:
        return "ui"
    if "/lua/" in normalized:
        return "lua"
    if "/simulation/" in normalized and "/simulation/elements/" not in normalized:
        return "simulation_core"
    if any(token in normalized for token in ("config.h", "elementnumbers", "simulationconfig")):
        return "protocol"
    return "other"


def compare(repo_id: str, repo: Path, official: Path) -> dict[str, object]:
    changed: list[dict[str, str]] = []
    counts: dict[str, int] = {}
    for path in source_files(repo, {".cpp", ".cc", ".c", ".h", ".hpp"}):
        relative = path.relative_to(repo).as_posix()
        kind = category(relative)
        if kind == "other" or "/elements/" in f"/{relative.lower()}/":
            continue
        baseline = official / relative
        status = "added" if not baseline.is_file() else ("modified" if sha256(path) != sha256(baseline) else "same")
        if status == "same":
            continue
        counts[kind] = counts.get(kind, 0) + 1
        changed.append({"file": relative, "category": kind, "status": status})
    return {
        "mod_id": repo_id,
        "core_files_modified": len(changed),
        "category_counts": counts,
        "save_format_modified": bool(counts.get("save") or counts.get("protocol")),
        "network_code_modified": bool(counts.get("network")),
        "ui_modified": bool(counts.get("ui")),
        "simulation_core_modified": bool(counts.get("simulation_core")),
        "files": changed,
    }


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--official", type=Path)
    parser.add_argument("--repository", action="append", default=[])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    repositories = root / "external/repositories"
    official = (args.official or repositories / "official").resolve()
    if not (official / ".git").exists():
        raise SystemExit("official comparison repository is missing")
    scanned = read_json(root / "external/metadata/repository_scan.json", [])
    repo_ids = args.repository or [row["mod_id"] for row in scanned if row["audit_status"] == "scanned" and row["mod_id"] != "official"]
    records = [
        compare(repo_id, repositories / repo_id, official)
        for repo_id in repo_ids
        if (repositories / repo_id / ".git").exists()
    ]
    records.sort(key=lambda row: str(row["mod_id"]))
    output = args.output or root / "external/metadata/core_modifications.json"
    write_json(output, records)
    print(f"detect-core-modifications: PASS repositories={len(records)} output={output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
