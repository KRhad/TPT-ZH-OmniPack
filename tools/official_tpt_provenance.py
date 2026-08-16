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
from urllib.error import HTTPError, URLError
from urllib.request import HTTPRedirectHandler, Request, build_opener


EVIDENCE_SCHEMA = "omnipack-release-evidence"
MANIFEST_SCHEMA = "omnipack-official-tpt-save-corpus-v1"
MANIFEST_SCHEMA_V2 = "omnipack-official-tpt-save-corpus-v2"
OFFICIAL_REPOSITORY = "https://github.com/The-Powder-Toy/The-Powder-Toy"
OFFICIAL_TPT_BENCH_REPOSITORY = "https://github.com/The-Powder-Toy/tpt-bench"
OFFICIAL_GIT_REPOSITORIES = {OFFICIAL_REPOSITORY, OFFICIAL_TPT_BENCH_REPOSITORY}
OFFICIAL_WEB_API_ORIGIN = "https://powdertoy.co.uk"
OFFICIAL_WEB_STATIC_ORIGIN = "https://static.powdertoy.co.uk"
# This is deliberately a small, explicit trust set.  A server-side "Mod"
# field alone is not enough to turn arbitrary community saves into official
# release fixtures.
OFFICIAL_MAINTAINERS = {"jacob1", "Simon"}
# Pinned maintainer fixtures for this release.  Keeping IDs and historical
# dates in code prevents a manifest author from selecting an arbitrary current
# save merely because its account currently reports "Mod".
OFFICIAL_WEB_SAVE_ALLOWLIST = {
    1249335: {"username": "jacob1", "date": 1738891791, "date_created": 1372986719},
    284: {"username": "Simon", "date": 1276381701, "date_created": 1276381701},
    1101197: {"username": "jacob1", "date": 1427430299, "date_created": 1361045969},
}
ALLOWED_REDISTRIBUTION = {"local_only_not_for_redistribution", "redistribution_permitted"}
SHA256_RE = re.compile(r"^[0-9A-Fa-f]{64}$")
REVISION_RE = re.compile(r"^[0-9A-Fa-f]{40}$")
REMOTE_NAME_RE = re.compile(r"^[A-Za-z0-9._-]+$")


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
    def __init__(
        self,
        repository: Path,
        git: str,
        remote: str,
        source_url: str | None = None,
    ) -> None:
        self.repository = repository
        self.git = git
        self.remote = remote
        self._pinned_source_url = source_url is not None
        self.source_url = normalized_repository(source_url or OFFICIAL_REPOSITORY)
        if not REMOTE_NAME_RE.fullmatch(self.remote):
            raise ValueError("Git provenance remote namespace is invalid")

    def _run(self, *args: str, text: bool = False, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [self.git, "-C", str(self.repository), *args],
            check=check,
            capture_output=True,
            text=text,
        )

    def repository_url(self) -> str:
        if self._pinned_source_url:
            return self.source_url
        return self._run("remote", "get-url", self.remote, text=True).stdout.strip()

    def fetch(self) -> None:
        # Never import official tag names into refs/tags: this integration repo
        # may legitimately have same-named local tags.  Isolate all provenance
        # refs under the remote namespace so a collision cannot block or spoof
        # the fetch.  Fetch from the pinned official URL instead of the mutable
        # local remote alias: a concurrent config change between repository_url
        # validation and this call must not redirect provenance objects.
        self._run(
            "fetch", "--prune", "--no-tags", self.source_url + ".git",
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


class _RejectRedirects(HTTPRedirectHandler):
    """Keep provenance downloads on the exact pinned official host and URL."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):  # type: ignore[no-untyped-def]
        return None


def fetch_exact_https(url: str, *, accept: str) -> bytes:
    """Fetch one exact HTTPS URL and reject every redirect or non-HTTPS hop."""
    if not url.startswith("https://"):
        raise ValueError("provenance URL must use HTTPS")
    opener = build_opener(_RejectRedirects())
    request = Request(url, headers={"Accept": accept, "User-Agent": "OmniPack-release-audit/1"})
    with opener.open(request, timeout=45) as response:
        if response.geturl() != url:
            raise ValueError("provenance response URL differs from pinned URL")
        return response.read()


def _v2_expected_git_locator(repository: str, revision: str, path: str) -> set[str]:
    return {
        f"{repository}/blob/{revision}/{path}",
        f"https://raw.githubusercontent.com/{repository.removeprefix('https://github.com/')}/{revision}/{path}",
    }


def _v2_source_revision(row: dict[str, object]) -> str | None:
    revision = row.get("source_revision")
    return revision if isinstance(revision, str) and REVISION_RE.fullmatch(revision) else None


def validate_provenance_v2(
    corpus: Path,
    manifest: dict[str, object],
    repository: Path,
    git: str,
    remote: str,
    *,
    run_id: str,
    commit: str,
) -> dict[str, object]:
    """Validate mixed official Git blobs and explicitly separated web saves.

    The web source is intentionally not represented as a Git object.  It is
    accepted only for a small allowlisted set of maintainer accounts, with
    live API metadata, exact ID/date/host locators, no redirects and a fresh
    content hash.  The corpus must still contain at least one real official
    Git fixture, preserving the original provenance requirement.
    """
    result = base_result(run_id, commit, "FAIL", "v2 provenance validation failed")
    result.update({
        "provenance_schema": MANIFEST_SCHEMA_V2,
        "repository": None,
        "revision": None,
        "source_repositories": [],
        "repositories": [],
    })
    if manifest.get("schema") != MANIFEST_SCHEMA_V2:
        result["reason"] = "manifest schema is invalid"
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
    if (
        not isinstance(redistribution, dict)
        or redistribution.get("status") not in ALLOWED_REDISTRIBUTION
        or not str(redistribution.get("basis", "")).strip()
    ):
        result["reason"] = "redistribution boundary is absent or invalid"
        return result
    if redistribution.get("status") != "local_only_not_for_redistribution":
        result["reason"] = "mixed official corpus must remain local-only"
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
    manifest_paths: list[str] = []
    records: list[dict[str, object]] = []
    failed = 0
    git_groups: dict[tuple[str, str], list[dict[str, object]]] = {}
    web_rows: list[dict[str, object]] = []
    for row in rows:
        if not isinstance(row, dict):
            records.append({"path": None, "match": False, "reason": "file row is not an object"})
            failed += 1
            continue
        kind = row.get("source_kind")
        if kind == "github_git":
            source_repository = normalized_repository(str(row.get("source_repository", "")))
            revision = _v2_source_revision(row)
            if source_repository not in OFFICIAL_GIT_REPOSITORIES or revision is None:
                records.append({"path": row.get("path"), "source_kind": kind, "match": False, "reason": "Git source identity is invalid"})
                failed += 1
                continue
            git_groups.setdefault((source_repository, revision), []).append(row)
        elif kind == "official_web_save":
            web_rows.append(row)
        else:
            records.append({"path": row.get("path"), "source_kind": kind, "match": False, "reason": "source_kind is not allowlisted"})
            failed += 1

    repository_records: list[dict[str, object]] = []
    git_verified = 0
    for group_index, ((source_repository, revision), group_rows) in enumerate(sorted(git_groups.items())):
        namespace = f"omnipack-v2-{group_index}-{revision[:8]}"
        try:
            upstream = GitUpstream(repository, git, namespace, source_url=source_repository)
            upstream.fetch()
            revision_exists = upstream.revision_exists(revision)
            revision_reachable = revision_exists and upstream.revision_reachable(revision)
        except (OSError, ValueError, subprocess.SubprocessError):
            revision_exists = False
            revision_reachable = False
            upstream = None
        repository_records.append({
            "source_kind": "github_git",
            "repository": source_repository,
            "revision": revision,
            "remote_namespace": namespace,
            "revision_exists": revision_exists,
            "revision_reachable_from_official_remote": revision_reachable,
        })
        for row in group_rows:
            path = safe_fixture_path(row.get("path"))
            source_path = safe_fixture_path(row.get("source_path"))
            manifest_sha = row.get("sha256")
            record: dict[str, object] = {
                "path": path or row.get("path"),
                "source_kind": "github_git",
                "source_repository": source_repository,
                "source_revision": revision,
                "source_path": source_path or row.get("source_path"),
                "source_locator": row.get("source_locator"),
                "match": False,
            }
            records.append(record)
            if path is None or source_path is None or not isinstance(manifest_sha, str) or not SHA256_RE.fullmatch(manifest_sha):
                record["reason"] = "path, source_path or manifest SHA256 is invalid"
                failed += 1
                continue
            manifest_paths.append(path)
            locator = row.get("source_locator")
            if locator not in _v2_expected_git_locator(source_repository, revision, source_path):
                record["reason"] = "Git source locator does not exactly match source path"
                failed += 1
                continue
            fixture = corpus / Path(path)
            if not fixture.is_file() or fixture.is_symlink():
                record["reason"] = "local fixture is absent or is a symlink"
                failed += 1
                continue
            try:
                fixture.resolve().relative_to(corpus.resolve())
                if upstream is None or not revision_exists or not revision_reachable:
                    raise FileNotFoundError(source_path)
                upstream_bytes = upstream.blob(revision, source_path)
            except (ValueError, FileNotFoundError, OSError, subprocess.SubprocessError):
                record["reason"] = "upstream Git object path is absent or unreachable"
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
            if match:
                git_verified += 1
            else:
                record["reason"] = "upstream, manifest and fixture hashes do not all match"
                failed += 1

    for row in web_rows:
        path = safe_fixture_path(row.get("path"))
        record = {
            "path": path or row.get("path"),
            "source_kind": "official_web_save",
            "source_repository": OFFICIAL_WEB_API_ORIGIN,
            "source_locator": row.get("source_locator"),
            "content_locator": row.get("content_locator"),
            "match": False,
        }
        records.append(record)
        save_id = row.get("save_id")
        if path is None or not isinstance(save_id, int) or isinstance(save_id, bool) or save_id <= 0:
            record["reason"] = "web save path or ID is invalid"
            failed += 1
            continue
        manifest_sha = row.get("sha256")
        expected_metadata = row.get("metadata")
        expected_date = row.get("source_date")
        expected_api = f"{OFFICIAL_WEB_API_ORIGIN}/Browse/View.json?ID={save_id}"
        expected_content = f"{OFFICIAL_WEB_STATIC_ORIGIN}/{save_id}{Path(path).suffix.lower()}"
        if (
            not isinstance(manifest_sha, str)
            or SHA256_RE.fullmatch(manifest_sha) is None
            or not isinstance(expected_metadata, dict)
            or not isinstance(expected_date, int)
            or row.get("source_repository") != OFFICIAL_WEB_API_ORIGIN
            or row.get("source_locator") != expected_api
            or row.get("content_locator") != expected_content
        ):
            record["reason"] = "web source locator, date or manifest metadata is invalid"
            failed += 1
            continue
        manifest_paths.append(path)
        fixture = corpus / Path(path)
        if not fixture.is_file() or fixture.is_symlink():
            record["reason"] = "local fixture is absent or is a symlink"
            failed += 1
            continue
        try:
            fixture.resolve().relative_to(corpus.resolve())
            metadata_bytes = fetch_exact_https(expected_api, accept="application/json")
            metadata = json.loads(metadata_bytes.decode("utf-8"))
            content_bytes = fetch_exact_https(expected_content, accept="application/vnd.powdertoy.save,application/octet-stream")
        except (OSError, ValueError, UnicodeError, json.JSONDecodeError, HTTPError, URLError):
            record["reason"] = "official web metadata or save fetch failed"
            failed += 1
            continue
        actual_metadata = {
            "id": metadata.get("ID") if isinstance(metadata, dict) else None,
            "username": metadata.get("Username") if isinstance(metadata, dict) else None,
            "elevation": metadata.get("Elevation") if isinstance(metadata, dict) else None,
            "published": metadata.get("Published") if isinstance(metadata, dict) else None,
            "date": metadata.get("Date") if isinstance(metadata, dict) else None,
            "date_created": metadata.get("DateCreated") if isinstance(metadata, dict) else None,
            "is_banned": metadata.get("IsBanned") if isinstance(metadata, dict) else None,
        }
        expected_subset = {key: expected_metadata.get(key) for key in actual_metadata}
        allowlisted_metadata = OFFICIAL_WEB_SAVE_ALLOWLIST.get(save_id)
        metadata_ok = (
            allowlisted_metadata is not None
            and actual_metadata == expected_subset
            and actual_metadata["id"] == save_id
            and actual_metadata["username"] in OFFICIAL_MAINTAINERS
            and actual_metadata["username"] == allowlisted_metadata["username"]
            and actual_metadata["elevation"] == "Mod"
            and actual_metadata["published"] is True
            and actual_metadata["is_banned"] is False
            and actual_metadata["date"] == expected_date
            and actual_metadata["date"] == allowlisted_metadata["date"]
            and actual_metadata["date_created"] == allowlisted_metadata["date_created"]
        )
        upstream_sha = digest_bytes(content_bytes)
        fixture_sha = digest_file(fixture)
        manifest_sha = manifest_sha.upper()
        match = metadata_ok and upstream_sha == manifest_sha == fixture_sha
        record.update({
            "source_date": expected_date,
            "save_id": save_id,
            "metadata": actual_metadata,
            "upstream_sha256": upstream_sha,
            "manifest_sha256": manifest_sha,
            "fixture_sha256": fixture_sha,
            "match": match,
        })
        repository_records.append({
            "source_kind": "official_web_save",
            "repository": OFFICIAL_WEB_API_ORIGIN,
            "save_id": save_id,
            "source_date": expected_date,
            "metadata_verified": metadata_ok,
            "content_hash_verified": upstream_sha == manifest_sha == fixture_sha,
        })
        if not match:
            record["reason"] = "official web metadata or content hash does not match manifest and fixture"
            failed += 1

    if sorted(manifest_paths) != actual_paths or len(manifest_paths) != len(set(manifest_paths)):
        result["reason"] = "manifest inventory does not exactly match local corpus"
        failed += 1
    if git_verified == 0:
        result["reason"] = "v2 corpus contains no verified official Git fixture"
        failed += 1
    result.update({
        "source_repositories": sorted({str(item["repository"]) for item in repository_records}),
        "repositories": repository_records,
        "files": records,
        "files_total": len(rows),
        "files_failed": failed,
        "files_verified": len(records) - failed if failed <= len(records) else 0,
        "revision_exists": bool(repository_records) and all(
            item.get("revision_exists") is True for item in repository_records if item.get("source_kind") == "github_git"
        ),
        "revision_reachable_from_official_remote": bool(repository_records) and all(
            item.get("revision_reachable_from_official_remote") is True
            for item in repository_records if item.get("source_kind") == "github_git"
        ),
    })
    if failed == 0 and len(records) == len(rows) and git_verified > 0:
        result.update(status="PASS", passed=True, reason="all mixed official Git/web fixtures match pinned provenance")
    elif result.get("reason") == "v2 provenance validation failed":
        result["reason"] = "one or more mixed official fixtures failed provenance"
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
            manifest_object = manifest if isinstance(manifest, dict) else {}
            if manifest_object.get("schema") == MANIFEST_SCHEMA_V2:
                result = validate_provenance_v2(
                    args.corpus,
                    manifest_object,
                    args.repository,
                    args.git,
                    args.remote,
                    run_id=args.run_id,
                    commit=args.commit,
                )
            else:
                result = validate_provenance(
                    args.corpus,
                    manifest_object,
                    GitUpstream(args.repository, args.git, args.remote),
                    run_id=args.run_id,
                    commit=args.commit,
                )
    write(args.output, result)
    return 0 if result["status"] == "PASS" else (2 if result["status"] == "NOT_TESTED" else 1)


if __name__ == "__main__":
    raise SystemExit(main())
