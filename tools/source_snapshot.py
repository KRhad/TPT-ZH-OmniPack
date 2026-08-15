#!/usr/bin/env python3
"""Create deterministic snapshots of source and selected ignored build inputs.

The release driver uses this at several checkpoints.  A Git commit and a
porcelain status are useful identity signals, but neither is a content
snapshot of the files that the compiler actually read.  This helper hashes
all tracked files plus non-ignored untracked files in a canonical order.  A
release build may additionally name Meson wrap files whose downloaded archive
and extracted directory are compiler inputs.  Those ignored bytes are verified
against the wrap ``source_hash`` and folded into a separate build-input digest.
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
from typing import Sequence
import zipfile


def _git(root: Path, *args: str) -> bytes:
    completed = subprocess.run(
        ["git", *args], cwd=root, check=False, capture_output=True
    )
    if completed.returncode:
        raise RuntimeError(completed.stderr.decode("utf-8", errors="replace").strip())
    return completed.stdout


def _nul_paths(data: bytes) -> list[str]:
    return [item.decode("utf-8", errors="strict") for item in data.split(b"\0") if item]


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def _tree_digest(entries: dict[str, tuple[int, str]]) -> str:
    digest = hashlib.sha256()
    digest.update(b"TPT-ZH-OmniPack normalized dependency tree v1\0")
    for relative, (size, file_hash) in sorted(entries.items()):
        encoded = relative.encode("utf-8")
        digest.update(len(encoded).to_bytes(4, "big"))
        digest.update(encoded)
        digest.update(size.to_bytes(8, "big"))
        digest.update(bytes.fromhex(file_hash))
    return digest.hexdigest().upper()


def _archive_tree(archive: Path, directory: str) -> tuple[dict[str, tuple[int, str]], str]:
    entries: dict[str, tuple[int, str]] = {}
    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
        if len(names) != len(set(names)):
            raise RuntimeError(f"dependency archive repeats ZIP members: {archive}")
        prefix = PurePosixPath(directory)
        for info in bundle.infolist():
            raw = info.filename
            if "\\" in raw:
                raise RuntimeError(f"dependency archive has a backslash path: {raw}")
            path = PurePosixPath(raw)
            if path.is_absolute() or ".." in path.parts or not path.parts:
                raise RuntimeError(f"dependency archive has an unsafe path: {raw}")
            if path.parts[0] != prefix.name:
                raise RuntimeError(f"dependency archive escapes declared directory: {raw}")
            if info.is_dir():
                continue
            mode = (info.external_attr >> 16) & 0o170000
            if mode == 0o120000:
                raise RuntimeError(f"dependency archive contains a symlink: {raw}")
            relative = PurePosixPath(*path.parts[1:]).as_posix()
            if not relative or relative in entries:
                raise RuntimeError(f"dependency archive has an invalid member: {raw}")
            data = bundle.read(info)
            entries[relative] = (len(data), hashlib.sha256(data).hexdigest().upper())
    if not entries:
        raise RuntimeError(f"dependency archive has no files: {archive}")
    return entries, _tree_digest(entries)


def _directory_tree(directory: Path) -> tuple[dict[str, tuple[int, str]], str]:
    entries: dict[str, tuple[int, str]] = {}
    if directory.is_symlink():
        raise RuntimeError(f"dependency directory is a symlink: {directory}")
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise RuntimeError(f"dependency tree contains a symlink: {path}")
        if not path.is_file() or path.name == ".meson-subproject-wrap-hash.txt":
            continue
        relative = path.relative_to(directory).as_posix()
        entries[relative] = (path.stat().st_size, _sha256_file(path))
    if not entries:
        raise RuntimeError(f"dependency directory has no files: {directory}")
    return entries, _tree_digest(entries)


def _safe_relative(raw: str) -> str:
    if not raw or "\\" in raw:
        raise RuntimeError(f"unsafe required wrap path: {raw!r}")
    path = PurePosixPath(raw)
    if path.is_absolute() or ".." in path.parts or path.suffix != ".wrap":
        raise RuntimeError(f"unsafe required wrap path: {raw!r}")
    return path.as_posix()


def _build_input(root: Path, raw_wrap: str) -> dict[str, object]:
    relative_wrap = _safe_relative(raw_wrap)
    wrap = root / Path(relative_wrap)
    if not wrap.is_file() or wrap.is_symlink():
        raise RuntimeError(f"required Meson wrap is absent or unsafe: {relative_wrap}")
    tracked = set(_nul_paths(_git(root, "ls-files", "-z", "--", relative_wrap)))
    if relative_wrap not in tracked:
        raise RuntimeError(f"required Meson wrap is not tracked: {relative_wrap}")
    parser = configparser.ConfigParser(interpolation=None)
    try:
        parser.read_string(wrap.read_text(encoding="utf-8"))
        section = parser["wrap-file"]
        directory = section["directory"].strip()
        source_filename = section["source_filename"].strip()
        source_hash = section["source_hash"].strip().upper()
    except (OSError, UnicodeError, KeyError, configparser.Error) as exc:
        raise RuntimeError(f"required Meson wrap is invalid: {relative_wrap}") from exc
    if (
        PurePosixPath(directory).name != directory
        or PurePosixPath(source_filename).name != source_filename
        or len(source_hash) != 64
        or any(character not in "0123456789ABCDEF" for character in source_hash)
    ):
        raise RuntimeError(f"required Meson wrap fields are unsafe: {relative_wrap}")
    archive = root / "subprojects" / "packagecache" / source_filename
    extracted = root / "subprojects" / directory
    archive_present = archive.is_file() and not archive.is_symlink()
    directory_present = extracted.is_dir() and not extracted.is_symlink()
    archive_sha = _sha256_file(archive) if archive_present else None
    archive_matches = archive_sha == source_hash
    archive_entries: dict[str, tuple[int, str]] = {}
    directory_entries: dict[str, tuple[int, str]] = {}
    archive_tree_sha = None
    directory_tree_sha = None
    extraction_matches = False
    reason = None
    try:
        if not archive_present:
            raise RuntimeError("dependency archive is absent")
        if not archive_matches:
            raise RuntimeError("dependency archive SHA256 does not match wrap source_hash")
        if not directory_present:
            raise RuntimeError("dependency extracted directory is absent")
        archive_entries, archive_tree_sha = _archive_tree(archive, directory)
        directory_entries, directory_tree_sha = _directory_tree(extracted)
        extraction_matches = archive_entries == directory_entries
        if not extraction_matches:
            raise RuntimeError("dependency extracted bytes do not match the verified archive")
    except (OSError, RuntimeError, zipfile.BadZipFile) as exc:
        reason = str(exc)
    ready = bool(archive_matches and extraction_matches)
    return {
        "wrap": relative_wrap,
        "directory": directory,
        "source_filename": source_filename,
        "source_hash": source_hash,
        "archive_present": archive_present,
        "archive_sha256": archive_sha,
        "archive_hash_matches": archive_matches,
        "directory_present": directory_present,
        "archive_tree_sha256": archive_tree_sha,
        "extracted_tree_sha256": directory_tree_sha,
        "extracted_matches_archive": extraction_matches,
        "files_total": len(archive_entries),
        "ready": ready,
        "reason": reason,
    }


def _build_inputs_digest(records: Sequence[dict[str, object]]) -> str:
    digest = hashlib.sha256()
    digest.update(b"TPT-ZH-OmniPack build input snapshot v1\0")
    for record in sorted(records, key=lambda item: str(item["wrap"])):
        for field in (
            "wrap", "directory", "source_filename", "source_hash",
            "archive_sha256", "archive_tree_sha256", "extracted_tree_sha256",
            "files_total", "ready",
        ):
            encoded = json.dumps(record.get(field), ensure_ascii=False, sort_keys=True).encode("utf-8")
            digest.update(len(encoded).to_bytes(4, "big"))
            digest.update(encoded)
    return digest.hexdigest().upper()


def snapshot(root: Path, required_wraps: Sequence[str] = ()) -> dict[str, object]:
    root = root.resolve()
    tracked = sorted(set(_nul_paths(_git(root, "ls-files", "-z"))))
    untracked = sorted(set(_nul_paths(_git(
        root, "ls-files", "--others", "--exclude-standard", "-z", "--", "."
    ))))
    digest = hashlib.sha256()
    digest.update(b"TPT-ZH-OmniPack full worktree snapshot v3\0")
    rows: list[tuple[str, str]] = []
    for kind, paths in (("tracked", tracked), ("untracked", untracked)):
        for relative in paths:
            rows.append((kind, relative))
    for kind, relative in rows:
        encoded = relative.encode("utf-8")
        path = root / Path(relative)
        exists = path.is_file()
        data = path.read_bytes() if exists else b""
        digest.update(b"T" if kind == "tracked" else b"U")
        digest.update(len(encoded).to_bytes(4, "big"))
        digest.update(encoded)
        digest.update(b"F" if exists else b"D")
        digest.update(len(data).to_bytes(8, "big"))
        digest.update(data)
    status = _git(root, "status", "--porcelain=v1", "--untracked-files=all").decode(
        "utf-8", errors="replace"
    ).strip("\r\n")
    build_inputs = [_build_input(root, value) for value in sorted(set(required_wraps))]
    return {
        "schema": "omnipack-source-snapshot",
        "schema_version": 1,
        "snapshot_format": 4,
        "source_state": "clean" if not status else "dirty",
        "source_worktree_sha256": digest.hexdigest().upper(),
        "source_untracked_files": len(untracked),
        "tracked_files": len(tracked),
        "untracked_files": len(untracked),
        "porcelain_output": status,
        "build_inputs_sha256": _build_inputs_digest(build_inputs),
        "build_inputs_ready": all(bool(record["ready"]) for record in build_inputs),
        "build_inputs_total": len(build_inputs),
        "build_inputs": build_inputs,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--required-wrap", action="append", default=[],
        help="Tracked Meson wrap whose archive and extracted bytes are mandatory build inputs.",
    )
    args = parser.parse_args(argv)
    try:
        value = snapshot(args.source_root, args.required_wrap)
        encoded = json.dumps(value, ensure_ascii=False, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(encoded, encoding="utf-8", newline="\n")
        else:
            sys.stdout.write(encoded)
        return 0
    except (OSError, RuntimeError, UnicodeError) as exc:
        print(f"source-snapshot: ERROR {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
