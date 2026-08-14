#!/usr/bin/env python3
"""Fail-closed inventory and round-trip contract for official TPT save corpus."""

from __future__ import annotations

import argparse
from datetime import date
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
from typing import Sequence


MANIFEST_SCHEMA = "omnipack-official-tpt-save-corpus-v1"
OFFICIAL_REPOSITORY = "https://github.com/The-Powder-Toy/The-Powder-Toy"
ALLOWED_REDISTRIBUTION = {
    "local_only_not_for_redistribution",
    "redistribution_permitted",
}
SHA256_RE = re.compile(r"^[0-9A-Fa-f]{64}$")
REVISION_RE = re.compile(r"^[0-9A-Fa-f]{40}$")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def write(path: Path, value: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def result(
    output: Path,
    *,
    status: str,
    reason: str,
    files_tested: int = 0,
    files_failed: int = 0,
    **extra: object,
) -> int:
    write(output, {
        "test": "official_tpt_save_compatibility",
        "passed": status == "PASS",
        "status": status,
        "reason": reason,
        "files_tested": files_tested,
        "files_failed": files_failed,
        **extra,
    })
    return 0 if status == "PASS" else (2 if status == "NOT_TESTED" else 1)


def validate_manifest(
    corpus: Path,
    manifest_path: Path,
    fixtures: list[Path],
) -> tuple[dict[str, object] | None, str | None]:
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        return None, f"provenance manifest is unreadable or invalid JSON: {type(exc).__name__}"
    if not isinstance(manifest, dict):
        return None, "provenance manifest root must be an object"
    if manifest.get("schema") != MANIFEST_SCHEMA:
        return None, f"provenance manifest schema must be {MANIFEST_SCHEMA}"
    corpus_id = manifest.get("corpus_id")
    if not isinstance(corpus_id, str) or not corpus_id.strip():
        return None, "provenance manifest corpus_id must be non-empty"
    if manifest.get("source_repository") != OFFICIAL_REPOSITORY:
        return None, "provenance source_repository is not the official TPT repository"
    revision = manifest.get("source_revision")
    if not isinstance(revision, str) or not REVISION_RE.fullmatch(revision):
        return None, "provenance source_revision must be a full 40-character Git commit"
    retrieved_at = manifest.get("retrieved_at")
    try:
        retrieved_date = date.fromisoformat(retrieved_at) if isinstance(retrieved_at, str) else None
    except ValueError:
        retrieved_date = None
    if retrieved_date is None or retrieved_date > date.today():
        return None, "provenance retrieved_at must be a non-future ISO date"
    redistribution = manifest.get("redistribution")
    if not isinstance(redistribution, dict):
        return None, "provenance redistribution must be an object"
    redistribution_status = redistribution.get("status")
    if redistribution_status not in ALLOWED_REDISTRIBUTION:
        return None, "provenance redistribution status is absent or unsupported"
    if not isinstance(redistribution.get("basis"), str) or not redistribution["basis"].strip():
        return None, "provenance redistribution basis must be non-empty"
    rows = manifest.get("files")
    if not isinstance(rows, list) or not rows:
        return None, "provenance files must be a non-empty array"

    expected: dict[str, str] = {}
    revision_lower = revision.lower()
    allowed_locator_prefixes = (
        f"{OFFICIAL_REPOSITORY.lower()}/blob/{revision_lower}/",
        f"https://raw.githubusercontent.com/The-Powder-Toy/The-Powder-Toy/{revision_lower}/",
    )
    allowed_locator_prefixes = tuple(prefix.lower() for prefix in allowed_locator_prefixes)
    for row in rows:
        if not isinstance(row, dict):
            return None, "each provenance file entry must be an object"
        relative = row.get("path")
        digest = row.get("sha256")
        locator = row.get("source_locator")
        if not isinstance(relative, str):
            return None, "each provenance file path must be a string"
        relative_path = Path(relative)
        normalized = relative.replace("\\", "/")
        if (
            relative_path.is_absolute()
            or ".." in relative_path.parts
            or normalized.startswith("/")
            or relative_path.suffix.lower() not in {".cps", ".stm"}
            or normalized in expected
        ):
            return None, f"unsafe, duplicate, or unsupported provenance path: {relative}"
        if not isinstance(digest, str) or not SHA256_RE.fullmatch(digest):
            return None, f"invalid SHA-256 for provenance path: {relative}"
        if not isinstance(locator, str) or not locator.lower().startswith(allowed_locator_prefixes):
            return None, f"source locator is not bound to the recorded official revision: {relative}"
        expected[normalized] = digest.upper()

    actual = {
        str(path.relative_to(corpus)).replace("\\", "/"): sha256(path)
        for path in fixtures
    }
    if set(actual) != set(expected):
        return None, "provenance file inventory does not exactly match the corpus"
    for relative, digest in actual.items():
        if expected[relative] != digest:
            return None, f"provenance SHA-256 mismatch: {relative}"
    return manifest, None


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, required=True)
    parser.add_argument("--provenance-manifest", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    fixtures = sorted(
        path for path in args.corpus.glob("**/*")
        if path.is_file() and path.suffix.lower() in {".cps", ".stm"}
    ) if args.corpus.is_dir() else []
    if not fixtures:
        return result(args.output, status="NOT_TESTED", reason="official TPT save corpus is absent")
    if not args.provenance_manifest.is_file():
        return result(
            args.output,
            status="NOT_TESTED",
            reason="official TPT save provenance manifest is absent",
            files_failed=len(fixtures),
            provenance_valid=False,
        )
    manifest, manifest_error = validate_manifest(args.corpus, args.provenance_manifest, fixtures)
    if manifest_error or manifest is None:
        return result(
            args.output,
            status="FAIL",
            reason=manifest_error or "official TPT save provenance validation failed",
            files_failed=len(fixtures),
            provenance_valid=False,
            provenance_manifest_sha256=sha256(args.provenance_manifest),
        )
    if not args.probe.is_file():
        return result(
            args.output,
            status="FAIL",
            reason="compatibility probe is absent",
            files_failed=len(fixtures),
            provenance_valid=True,
            provenance_manifest_sha256=sha256(args.provenance_manifest),
        )
    records: list[dict[str, object]] = []
    failed = 0
    for fixture in fixtures:
        completed = subprocess.run(
            [str(args.probe), str(fixture)], check=False,
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        passed = completed.returncode == 0 and "official_save_roundtrip_pass=true" in completed.stdout
        failed += 0 if passed else 1
        records.append({
            "file": str(fixture.relative_to(args.corpus)).replace("\\", "/"),
            "sha256": sha256(fixture), "passed": passed,
            "exit_code": completed.returncode,
        })
    write(args.output, {
        "test": "official_tpt_save_compatibility", "passed": failed == 0,
        "status": "PASS" if failed == 0 else "FAIL",
        "reason": "all provenance-bound saves completed load, simulate, save, and reload" if failed == 0 else "one or more provenance-bound saves failed round-trip validation",
        "provenance_valid": True,
        "provenance_manifest": args.provenance_manifest.name,
        "provenance_manifest_sha256": sha256(args.provenance_manifest),
        "source_repository": manifest["source_repository"],
        "source_revision": manifest["source_revision"],
        "redistribution_status": manifest["redistribution"]["status"],
        "files_tested": len(fixtures), "files_failed": failed, "files": records,
    })
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
