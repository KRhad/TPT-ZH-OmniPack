#!/usr/bin/env python3
"""Verify an official TPT save corpus against real upstream Git objects."""

from __future__ import annotations

import argparse
from datetime import date
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
from typing import Protocol, Sequence


EVIDENCE_SCHEMA = "omnipack-release-evidence"
MANIFEST_SCHEMA = "omnipack-official-tpt-save-corpus-v1"
OFFICIAL_REPOSITORY = "https://github.com/The-Powder-Toy/The-Powder-Toy"
ALLOWED_REDISTRIBUTION = {"local_only_not_for_redistribution", "redistribution_permitted"}
SHA256_RE = re.compile(r"^[0-9A-Fa-f]{64}$")
REVISION_RE = re.compile(r"^[0-9A-Fa-f]{40}$")


def digest_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest().upper()


def digest_file(path: Path) -> str:
    return digest_bytes(path.read_bytes())


def write(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def normalized_repository(value: str) -> str:
    return value.removesuffix(".git").rstrip("/")


def safe_fixture_path(raw: object) -> str | None:
    if not isinstance(raw, str) or "\\" in raw:
        return None
    path = PurePosixPath(raw)
    if path.is_absolute() or ".." in path.parts or ":" in raw or path.suffix.lower() not in {".cps", ".stm"}:
        return None
    return str(path)


class Upstream(Protocol):
    def repository_url(self) -> str: ...
    def fetch(self) -> None: ...
    def revision_exists(self, revision: str) -> bool: ...
    def revision_reachable(self, revision: str) -> bool: ...
    def blob(self, revision: str, path: str) -> bytes: ...


class GitUpstream:
    def __init__(self, repository: Path, git: str, remote: str) -> None:
        self.repository = repository
        self.git = git
        self.remote = remote

    def _run(self, *args: str, text: bool = False, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [self.git, "-C", str(self.repository), *args],
            check=check,
            capture_output=True,
            text=text,
        )

    def repository_url(self) -> str:
        return self._run("remote", "get-url", self.remote, text=True).stdout.strip()

    def fetch(self) -> None:
        # Never import official tag names into refs/tags: this integration repo
        # may legitimately have same-named local tags.  Isolate all provenance
        # refs under the remote namespace so a collision cannot block or spoof
        # the fetch.  Fetch from the pinned official URL instead of the mutable
        # local remote alias: a concurrent config change between repository_url
        # validation and this call must not redirect provenance objects.
        self._run(
            "fetch", "--prune", OFFICIAL_REPOSITORY + ".git",
            "+refs/heads/*:refs/remotes/" + self.remote + "/*",
            "+refs/tags/*:refs/remotes/" + self.remote + "/tags/*",
        )

    def revision_exists(self, revision: str) -> bool:
        return self._run("cat-file", "-e", f"{revision}^{{commit}}", check=False).returncode == 0

    def revision_reachable(self, revision: str) -> bool:
        completed = self._run(
            "for-each-ref", "--format=%(refname)", f"--contains={revision}",
            f"refs/remotes/{self.remote}", text=True, check=False,
        )
        return completed.returncode == 0 and any(line.strip() for line in completed.stdout.splitlines())

    def blob(self, revision: str, path: str) -> bytes:
        completed = self._run("show", f"{revision}:{path}", check=False)
        if completed.returncode != 0:
            raise FileNotFoundError(path)
        return completed.stdout


def base_result(run_id: str, commit: str, status: str, reason: str) -> dict[str, object]:
    return {
        "schema": EVIDENCE_SCHEMA,
        "schema_version": 1,
        "test": "official_tpt_provenance",
        "run_id": run_id,
        "commit": commit,
        "status": status,
        "passed": status == "PASS",
        "reason": reason,
        "repository": OFFICIAL_REPOSITORY,
        "revision": None,
        "revision_exists": False,
        "revision_reachable_from_official_remote": False,
        "files_total": 0,
        "files_verified": 0,
        "files_failed": 0,
        "files": [],
    }


def validate_provenance(
    corpus: Path,
    manifest: dict[str, object],
    upstream: Upstream,
    *,
    run_id: str,
    commit: str,
) -> dict[str, object]:
    result = base_result(run_id, commit, "FAIL", "provenance validation failed")
    if manifest.get("schema") != MANIFEST_SCHEMA:
        result["reason"] = "manifest schema is invalid"
        return result
    if normalized_repository(str(manifest.get("source_repository", ""))) != OFFICIAL_REPOSITORY:
        result["reason"] = "manifest repository is not official TPT"
        return result
    try:
        remote_url = normalized_repository(upstream.repository_url())
    except (OSError, subprocess.SubprocessError):
        result["reason"] = "cannot resolve official Git remote URL"
        return result
    if remote_url != OFFICIAL_REPOSITORY:
        result["reason"] = "Git remote URL is not official TPT"
        return result
    revision = manifest.get("source_revision")
    result["revision"] = revision
    if not isinstance(revision, str) or not REVISION_RE.fullmatch(revision):
        result["reason"] = "source revision must be a full Git commit"
        return result
    retrieved_at = manifest.get("retrieved_at")
    try:
        retrieved = date.fromisoformat(retrieved_at) if isinstance(retrieved_at, str) else None
    except ValueError:
        retrieved = None
    if retrieved is None or retrieved > date.today():
        result["reason"] = "retrieved_at is absent, invalid, or in the future"
        return result
    redistribution = manifest.get("redistribution")
    if not isinstance(redistribution, dict) or redistribution.get("status") not in ALLOWED_REDISTRIBUTION or not str(redistribution.get("basis", "")).strip():
        result["reason"] = "redistribution boundary is absent or invalid"
        return result
    try:
        upstream.fetch()
    except (OSError, subprocess.SubprocessError):
        result["reason"] = "official Git fetch failed"
        return result
    result["revision_exists"] = upstream.revision_exists(revision)
    if not result["revision_exists"]:
        result["reason"] = "source revision does not exist as a commit"
        return result
    result["revision_reachable_from_official_remote"] = upstream.revision_reachable(revision)
    if not result["revision_reachable_from_official_remote"]:
        result["reason"] = "source revision is not reachable from official remote refs"
        return result

    rows = manifest.get("files")
    if not isinstance(rows, list) or not rows:
        result["reason"] = "manifest files are absent"
        return result
    actual_paths = sorted(
        str(path.relative_to(corpus)).replace("\\", "/")
        for path in corpus.rglob("*")
        if path.is_file() and path.suffix.lower() in {".cps", ".stm"}
    )
    result["files_total"] = len(rows)
    manifest_paths: list[str] = []
    records: list[dict[str, object]] = []
    failed = 0
    for row in rows:
        record: dict[str, object] = {"path": None, "match": False}
        records.append(record)
        if not isinstance(row, dict):
            record["reason"] = "file row is not an object"
            failed += 1
            continue
        path = safe_fixture_path(row.get("path"))
        record["path"] = path or row.get("path")
        manifest_sha = row.get("sha256")
        locator = row.get("source_locator")
        if path is None or not isinstance(manifest_sha, str) or not SHA256_RE.fullmatch(manifest_sha):
            record["reason"] = "path or manifest SHA256 is invalid"
            failed += 1
            continue
        manifest_paths.append(path)
        expected_locators = {
            f"{OFFICIAL_REPOSITORY}/blob/{revision}/{path}",
            f"https://raw.githubusercontent.com/The-Powder-Toy/The-Powder-Toy/{revision}/{path}",
        }
        if locator not in expected_locators:
            record["reason"] = "source locator path does not exactly match manifest path"
            failed += 1
            continue
        fixture = corpus / Path(path)
        if not fixture.is_file() or fixture.is_symlink():
            record["reason"] = "local fixture is absent or is a symlink"
            failed += 1
            continue
        try:
            fixture.resolve().relative_to(corpus.resolve())
            upstream_bytes = upstream.blob(revision, path)
        except (ValueError, FileNotFoundError, OSError, subprocess.SubprocessError):
            record["reason"] = "upstream Git object path is absent or unsafe"
            failed += 1
            continue
        upstream_sha = digest_bytes(upstream_bytes)
        fixture_sha = digest_file(fixture)
        manifest_sha = manifest_sha.upper()
        match = upstream_sha == manifest_sha == fixture_sha
        record.update({
            "upstream_sha256": upstream_sha,
            "manifest_sha256": manifest_sha,
            "fixture_sha256": fixture_sha,
            "match": match,
        })
        if not match:
            record["reason"] = "upstream, manifest, and fixture hashes do not all match"
            failed += 1
    if sorted(manifest_paths) != actual_paths or len(manifest_paths) != len(set(manifest_paths)):
        result["reason"] = "manifest inventory does not exactly match local corpus"
        failed += 1
    result["files"] = records
    result["files_failed"] = failed
    result["files_verified"] = len(records) - failed if failed <= len(records) else 0
    if failed == 0 and len(records) > 0:
        result.update(status="PASS", passed=True, reason="all fixtures match official upstream Git objects")
    else:
        result["reason"] = result.get("reason") if result.get("reason") != "provenance validation failed" else "one or more fixtures failed official Git provenance"
    return result


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--git", required=True)
    parser.add_argument("--remote", default="official")
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    if not args.corpus.is_dir() or not any(
        path.is_file() and path.suffix.lower() in {".cps", ".stm"} for path in args.corpus.rglob("*")
    ):
        result = base_result(args.run_id, args.commit, "NOT_TESTED", "official TPT save corpus is absent")
    elif not args.manifest.is_file():
        result = base_result(args.run_id, args.commit, "NOT_TESTED", "official TPT provenance manifest is absent")
    else:
        try:
            manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        except (OSError, UnicodeError, json.JSONDecodeError):
            result = base_result(args.run_id, args.commit, "FAIL", "provenance manifest is unreadable or invalid JSON")
        else:
            result = validate_provenance(
                args.corpus,
                manifest if isinstance(manifest, dict) else {},
                GitUpstream(args.repository, args.git, args.remote),
                run_id=args.run_id,
                commit=args.commit,
            )
    write(args.output, result)
    return 0 if result["status"] == "PASS" else (2 if result["status"] == "NOT_TESTED" else 1)


if __name__ == "__main__":
    raise SystemExit(main())
