from __future__ import annotations

import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
from typing import Any, Iterable


SOURCE_TYPES = {
    "cpp_source",
    "lua_source",
    "mixed_source",
    "binary_only",
    "documentation_only",
    "unavailable",
}

SOURCE_CATALOG_COLUMNS = (
    "mod_id", "mod_name", "author", "forum_url", "repository_url",
    "download_url", "source_type", "license", "license_verified",
    "base_tpt_version", "default_branch", "source_commit", "last_commit_date",
    "archived", "buildable", "element_count_claimed", "element_count_detected",
    "lua_script_count", "core_files_modified", "save_format_modified",
    "network_code_modified", "ui_modified", "known_bugs", "source_complete",
    "binary_only", "port_priority", "audit_status", "notes",
)

CPP_PATTERNS = (
    "src/simulation/elements/*.cpp",
    "src/simulation/elements/*.h",
    "src/simulation/Element*.cpp",
    "src/simulation/Element*.h",
    "src/Element*.cpp",
    "src/Element*.h",
    "elements/*.cpp",
    "elements/*.h",
)

IGNORED_PARTS = {".git", "build", "dist", "subprojects", "__pycache__"}


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return path.read_text(encoding="latin-1", errors="replace")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def read_json(path: Path, default: Any) -> Any:
    if not path.is_file():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def write_csv(path: Path, columns: Iterable[str], rows: Iterable[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(columns), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({column: row.get(column, "") for column in writer.fieldnames})


def run_git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return result.stdout.strip() if result.returncode == 0 else ""


def source_files(repo: Path, suffixes: set[str] | None = None) -> list[Path]:
    suffixes = suffixes or {".cpp", ".cc", ".c", ".h", ".hpp", ".lua"}
    files: list[Path] = []
    for path in repo.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in suffixes:
            continue
        if any(part in IGNORED_PARTS for part in path.relative_to(repo).parts):
            continue
        files.append(path)
    return sorted(files)


def classify_source(repo: Path) -> tuple[str, int, int]:
    cpp = source_files(repo, {".cpp", ".cc", ".c", ".h", ".hpp"})
    lua = source_files(repo, {".lua"})
    if cpp and lua:
        return "mixed_source", len(cpp), len(lua)
    if cpp:
        return "cpp_source", len(cpp), 0
    if lua:
        return "lua_source", 0, len(lua)
    documents = list(repo.glob("README*")) + list(repo.glob("*.md"))
    binaries = [path for path in repo.rglob("*") if path.suffix.lower() in {".exe", ".dll", ".app"}]
    if binaries:
        return "binary_only", 0, 0
    return ("documentation_only" if documents else "unavailable"), 0, 0


def detect_license(repo: Path) -> tuple[str, bool, list[str]]:
    candidates = sorted(
        path for path in repo.iterdir()
        if path.is_file() and re.fullmatch(r"(?i)(license|licence|copying)(\..*)?", path.name)
    )
    detected: set[str] = set()
    for path in candidates:
        text = read_text(path).lower()
        if "gnu general public license" in text and "version 3" in text:
            detected.add("GPL-3.0")
        elif "gnu general public license" in text and "version 2" in text:
            detected.add("GPL-2.0")
        elif "mit license" in text:
            detected.add("MIT")
        elif "apache license" in text and "version 2.0" in text:
            detected.add("Apache-2.0")
        elif "mozilla public license" in text:
            detected.add("MPL")
        else:
            detected.add("unclassified")
    value = "+".join(sorted(detected)) if detected else "unknown"
    return value, bool(candidates and detected - {"unclassified"}), [str(path.relative_to(repo)) for path in candidates]


def extract_version(repo: Path) -> str:
    meson = repo / "meson.build"
    if meson.is_file():
        match = re.search(
            r"project\s*\(.*?\bversion\s*:\s*['\"]([^'\"]+)['\"]",
            read_text(meson),
            re.I | re.S,
        )
        if match:
            return match.group(1)
    patterns = (
        re.compile(r"SAVE_VERSION\s+([0-9]+)"),
        re.compile(r"version\s*:\s*['\"]?([0-9]+(?:\.[0-9]+){1,3})", re.I),
        re.compile(r"(?:TPT|Powder Toy)[^\n]{0,40}([0-9]+\.[0-9]+(?:\.[0-9]+)?)", re.I),
    )
    for relative in ("src/Config.h", "README.md", "README"):
        path = repo / relative
        if not path.is_file():
            continue
        text = read_text(path)
        for pattern in patterns:
            match = pattern.search(text)
            if match:
                return match.group(1)
    return "unknown"


def balanced_block(text: str, opening: int) -> str:
    if opening < 0 or opening >= len(text) or text[opening] != "{":
        return ""
    depth = 0
    state = "code"
    index = opening
    while index < len(text):
        char = text[index]
        next_char = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == '"':
                state = "string"
            elif char == "'":
                state = "char"
            elif char == "/" and next_char == "/":
                state = "line_comment"
                index += 1
            elif char == "/" and next_char == "*":
                state = "block_comment"
                index += 1
            elif char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    return text[opening:index + 1]
        elif state == "string":
            if char == "\\":
                index += 1
            elif char == '"':
                state = "code"
        elif state == "char":
            if char == "\\":
                index += 1
            elif char == "'":
                state = "code"
        elif state == "line_comment" and char in "\r\n":
            state = "code"
        elif state == "block_comment" and char == "*" and next_char == "/":
            state = "code"
            index += 1
        index += 1
    return text[opening:]
