#!/usr/bin/env python3
"""Scan cloned TPT mod repositories and emit pinned source metadata."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
from typing import Sequence

from catalog_common import (
    SOURCE_CATALOG_COLUMNS,
    classify_source,
    detect_license,
    extract_version,
    read_text,
    run_git,
    sha256,
    source_files,
    write_csv,
    write_json,
)


IDENTIFIER = re.compile(
    r"\bIdentifier\s*=\s*[\"']([A-Za-z0-9_]+)[\"']|"
    r"(?:elements|elem)\.allocate\s*\(\s*[^,]+,\s*[\"']([^\"']+)[\"']"
)
LICENSE_FILE = re.compile(r"(?i)(license|licence|copying|notice)(\..*)?")
LICENSE_WORDS = re.compile(
    r"\blicen[cs]e(?:d|s)?\b|\bcopyright\b|\bGPL(?:v?[23](?:\.0)?)?\b|"
    r"\bMIT\b|\bApache(?:-2\.0)?\b|\bMozilla\b",
    re.I,
)
RESTRICTIVE_WORDS = re.compile(
    r"all rights reserved|non[- ]commercial|no redistribution|do not redistribute|proprietary",
    re.I,
)
ASSET_SUFFIXES = {
    ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".svg", ".ico",
    ".wav", ".ogg", ".mp3", ".ttf", ".otf", ".bdf",
}


def load_seeds(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def load_raw_sources(path: Path) -> dict[str, dict[str, str]]:
    if not path.is_file():
        return {}
    with path.open("r", encoding="utf-8", newline="") as stream:
        return {row["mod_id"]: row for row in csv.DictReader(stream)}


def detected_elements(repo: Path) -> int:
    identifiers: set[str] = set()
    for path in source_files(repo):
        text = read_text(path)
        for match in IDENTIFIER.finditer(text):
            identifiers.add(next(group for group in match.groups() if group))
    if identifiers:
        return len(identifiers)
    element_files = {
        path.stem
        for path in source_files(repo, {".cpp", ".cc", ".c"})
        if "element" in path.as_posix().lower() or "elements" in path.parts
    }
    return len(element_files)


def license_scope(repo: Path) -> dict[str, object]:
    tracked = [Path(line) for line in run_git(repo, "ls-files").splitlines() if line]
    readmes = [path for path in tracked if path.name.lower().startswith("readme")]
    element_sources: list[Path] = []
    for path in tracked:
        if path.suffix.lower() not in {".cpp", ".cc", ".c", ".h", ".hpp", ".lua"}:
            continue
        in_elements_directory = "elements" in {part.lower() for part in path.parts}
        lua_element_script = path.suffix.lower() == ".lua" and IDENTIFIER.search(read_text(repo / path))
        if in_elements_directory or lua_element_script:
            element_sources.append(path)
    assets = [path for path in tracked if path.suffix.lower() in ASSET_SUFFIXES]
    nested_notices = [path for path in tracked if len(path.parts) > 1 and LICENSE_FILE.fullmatch(path.name)]
    readme_mentions: list[str] = []
    for relative in readmes:
        for number, line in enumerate(read_text(repo / relative).splitlines(), 1):
            if LICENSE_WORDS.search(line):
                readme_mentions.append(f"{relative.as_posix()}:{number}:{' '.join(line.split())[:180]}")
                if len(readme_mentions) >= 8:
                    break
        if len(readme_mentions) >= 8:
            break
    header_notices: list[str] = []
    restrictive_files: list[str] = []
    for relative in element_sources:
        header = "\n".join(read_text(repo / relative).splitlines()[:80])
        if LICENSE_WORDS.search(header):
            header_notices.append(relative.as_posix())
        if RESTRICTIVE_WORDS.search(header):
            restrictive_files.append(relative.as_posix())
    gitmodules = repo / ".gitmodules"
    submodules = []
    if gitmodules.is_file():
        submodules = re.findall(r"(?m)^\s*path\s*=\s*(.+?)\s*$", read_text(gitmodules))
    return {
        "readme_files": [path.as_posix() for path in readmes],
        "readme_license_mentions": readme_mentions,
        "element_source_files_scanned": len(element_sources),
        "element_source_header_notices": header_notices,
        "element_source_restrictive_markers": restrictive_files,
        "asset_files_scanned": len(assets),
        "nested_notice_files": [path.as_posix() for path in nested_notices],
        "submodules": submodules,
    }


def scan_raw_lua(seed: dict[str, str], raw_root: Path, raw: dict[str, str]) -> dict[str, object] | None:
    path = raw_root / raw["relative_path"]
    if not path.is_file():
        return None
    digest = sha256(path)
    digest_matches = digest == raw["sha256"].upper()
    text = read_text(path)
    identifiers = {
        next(group for group in match.groups() if group)
        for match in IDENTIFIER.finditer(text)
    }
    header = "\n".join(text.splitlines()[:80])
    license_scope_audit = {
        "readme_files": [],
        "readme_license_mentions": [],
        "element_source_files_scanned": 1,
        "element_source_header_notices": [path.name] if LICENSE_WORDS.search(header) else [],
        "element_source_restrictive_markers": [path.name] if RESTRICTIVE_WORDS.search(header) else [],
        "asset_files_scanned": 0,
        "nested_notice_files": [],
        "submodules": [],
    }
    return {
        **seed,
        "mod_name": seed["mod_id"],
        "author": raw.get("author", "unknown"),
        "download_url": raw.get("download_url", seed.get("download_url", "")),
        "source_type": "lua_source",
        "license": raw.get("license", "unknown"),
        "license_verified": "false",
        "license_evidence": [],
        "license_scope_audit": license_scope_audit,
        "base_tpt_version": "unknown",
        "default_branch": "not_applicable",
        "source_commit": f"sha256:{digest}",
        "last_commit_date": raw.get("last_update_date", ""),
        "archived": "not_applicable",
        "buildable": "false",
        "element_count_claimed": "not_tested",
        "element_count_detected": len(identifiers),
        "cpp_source_file_count": 0,
        "lua_script_count": 1,
        "core_files_modified": 0,
        "save_format_modified": "false",
        "network_code_modified": "false",
        "ui_modified": "not_tested",
        "known_bugs": "not_reviewed",
        "source_complete": str(digest_matches).lower(),
        "binary_only": "false",
        "audit_status": "downloaded_source" if digest_matches else "checksum_mismatch",
        "path": (Path("external") / "lua-mods" / path.relative_to(raw_root).parent).as_posix(),
        "raw_source_file": path.name,
    }


def scan(
    seed: dict[str, str],
    repositories: Path,
    raw_root: Path | None = None,
    raw: dict[str, str] | None = None,
) -> dict[str, object]:
    mod_id = seed["mod_id"]
    repo = repositories / mod_id
    if not (repo / ".git").exists():
        if raw_root is not None and raw is not None:
            raw_record = scan_raw_lua(seed, raw_root, raw)
            if raw_record is not None:
                return raw_record
        return {
            **seed,
            "mod_name": mod_id,
            "author": seed["repository_url"].rstrip("/").split("/")[-2]
            if "github.com/" in seed["repository_url"] else "unknown",
            "download_url": seed.get("download_url", ""),
            "source_type": "unavailable",
            "license": "unknown",
            "license_verified": "false",
            "base_tpt_version": "unknown",
            "default_branch": "unknown",
            "source_commit": "",
            "last_commit_date": "",
            "archived": "not_tested",
            "buildable": "not_tested",
            "element_count_claimed": "not_tested",
            "element_count_detected": 0,
            "lua_script_count": 0,
            "core_files_modified": "not_tested",
            "save_format_modified": "not_tested",
            "network_code_modified": "not_tested",
            "ui_modified": "not_tested",
            "known_bugs": "not_reviewed",
            "source_complete": "false",
            "binary_only": "false",
            "audit_status": "not_cloned",
        }

    source_type, cpp_files, lua_files = classify_source(repo)
    license_name, license_verified, license_files = detect_license(repo)
    license_scope_audit = license_scope(repo)
    remote = run_git(repo, "remote", "get-url", "origin") or seed["repository_url"]
    branch = run_git(repo, "symbolic-ref", "--short", "refs/remotes/origin/HEAD")
    if branch.startswith("origin/"):
        branch = branch.removeprefix("origin/")
    if not branch:
        branch = run_git(repo, "branch", "--show-current") or "detached"
    commit = run_git(repo, "rev-parse", "HEAD")
    last_date = run_git(repo, "show", "-s", "--format=%cI", "HEAD")
    checkout_status = run_git(repo, "status", "--porcelain")
    checkout_complete = not checkout_status
    full_name = remote.removesuffix(".git").rstrip("/").split("github.com/")[-1]
    author = full_name.split("/", 1)[0] if "/" in full_name else "unknown"
    name = full_name.split("/", 1)[-1]
    count = detected_elements(repo)
    has_build = any((repo / name).exists() for name in ("meson.build", "SConstruct", "CMakeLists.txt"))
    return {
        **seed,
        "mod_name": name,
        "author": author,
        "repository_url": remote,
        "download_url": seed.get("download_url", ""),
        "source_type": source_type,
        "license": license_name,
        "license_verified": str(license_verified).lower(),
        "license_evidence": license_files,
        "license_scope_audit": license_scope_audit,
        "base_tpt_version": extract_version(repo),
        "default_branch": branch,
        "source_commit": commit,
        "last_commit_date": last_date,
        "archived": "not_tested",
        "buildable": "not_tested" if has_build else "false",
        "element_count_claimed": "not_tested",
        "element_count_detected": count,
        "cpp_source_file_count": cpp_files,
        "lua_script_count": lua_files,
        "core_files_modified": "not_tested",
        "save_format_modified": "not_tested",
        "network_code_modified": "not_tested",
        "ui_modified": "not_tested",
        "known_bugs": "not_reviewed",
        "source_complete": (
            "not_tested"
            if commit and checkout_complete and source_type not in {"binary_only", "unavailable"}
            else "false"
        ),
        "binary_only": str(source_type == "binary_only").lower(),
        "audit_status": (
            "scanned" if commit and checkout_complete
            else "checkout_incomplete" if commit
            else "empty_repository"
        ),
        "path": (Path("external") / "repositories" / mod_id).as_posix(),
    }


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--seeds", type=Path)
    parser.add_argument("--repositories", type=Path)
    parser.add_argument("--raw-lua-root", type=Path)
    parser.add_argument("--raw-lua-metadata", type=Path)
    parser.add_argument("--json-output", type=Path)
    parser.add_argument("--catalog-output", type=Path)
    args = parser.parse_args(argv)
    root = args.source_root.resolve()
    seeds_path = args.seeds or root / "external/metadata/source_seeds.csv"
    repositories = args.repositories or root / "external/repositories"
    raw_lua_root = args.raw_lua_root or root / "external/lua-mods"
    raw_sources = load_raw_sources(args.raw_lua_metadata or root / "external/metadata/raw_lua_sources.csv")
    json_output = args.json_output or root / "external/metadata/repository_scan.json"
    catalog_output = args.catalog_output or root / "docs/MOD_SOURCE_CATALOG.csv"
    rows = [
        scan(seed, repositories, raw_lua_root, raw_sources.get(seed["mod_id"]))
        for seed in load_seeds(seeds_path)
    ]
    rows.sort(key=lambda row: (int(row.get("port_priority", 9)), str(row["mod_id"])))
    write_json(json_output, rows)
    write_csv(catalog_output, SOURCE_CATALOG_COLUMNS, rows)
    cloned = sum(row["audit_status"] == "scanned" for row in rows)
    downloaded = sum(row["audit_status"] == "downloaded_source" for row in rows)
    print(
        f"scan-repositories: PASS seeds={len(rows)} cloned={cloned} "
        f"raw_downloads={downloaded} catalog={catalog_output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
