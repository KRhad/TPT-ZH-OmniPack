#!/usr/bin/env python3
"""Validate release-gate evidence integrity and documentation consistency.

This is deliberately fail-closed: a gate can only be considered auditable when
its evidence exists, is inside the validation directory, and matches the hash
recorded by the aggregate result.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import zipfile


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def write(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def evidence_path(root: Path, raw: object) -> Path | None:
    if not isinstance(raw, str) or not raw:
        return None
    candidate = Path(raw)
    path = candidate.resolve() if candidate.is_absolute() else (root / candidate).resolve()
    try:
        path.relative_to(root.resolve())
    except ValueError:
        return None
    return path if path.is_file() else None


def audit_evidence(validation: dict[str, object], root: Path) -> tuple[bool, list[dict[str, object]]]:
    rows: list[dict[str, object]] = []
    passed = True
    gates = validation.get("gates", {})
    if not isinstance(gates, dict):
        return False, [{"error": "gates is not an object"}]
    for name, value in gates.items():
        if not isinstance(value, dict):
            passed = False
            rows.append({"gate": name, "passed": False, "reason": "gate record is not an object"})
            continue
        raw = value.get("Evidence") or value.get("evidence")
        expected = value.get("EvidenceSha256") or value.get("evidence_sha256")
        path = evidence_path(root, raw)
        row: dict[str, object] = {"gate": name, "evidence": raw, "expected_sha256": expected}
        if path is None:
            row.update(passed=False, reason="evidence is absent or outside validation root")
            passed = False
        elif not isinstance(expected, str):
            # Legacy evidence records may point to a process-capture prefix. In
            # that case the JSON/hash pair is not yet auditable and must block.
            row.update(passed=False, reason="evidence_sha256 is absent")
            passed = False
        else:
            actual = sha256(path)
            row["actual_sha256"] = actual
            row["passed"] = actual == expected
            if actual != expected:
                row["reason"] = "evidence hash mismatch"
                passed = False
        rows.append(row)
    return passed, rows


def archive_text(package: Path) -> str:
    with zipfile.ZipFile(package) as archive:
        chunks = []
        for name in archive.namelist():
            if name.lower().endswith((".md", ".txt", ".json")):
                chunks.append(archive.read(name).decode("utf-8", errors="replace"))
        return "\n".join(chunks)


def audit_docs(validation: dict[str, object], validation_text: Path | None, build_info: Path | None,
               package: Path | None, channel: str) -> tuple[bool, list[str]]:
    errors: list[str] = []
    gates = validation.get("gates", {})
    if validation_text and validation_text.is_file():
        text = validation_text.read_text(encoding="utf-8", errors="replace")
        for name, record in gates.items() if isinstance(gates, dict) else []:
            status = record.get("Status") if isinstance(record, dict) else None
            if status and not re.search(rf"^{re.escape(name)}: {re.escape(status)}\b", text, re.MULTILINE):
                errors.append(f"validation text disagrees with JSON for {name}")
    if build_info and build_info.is_file():
        info = build_info.read_text(encoding="utf-8", errors="replace")
        if "1.1.0" not in info:
            errors.append("BUILD-INFO does not identify 1.1.0")
        if channel == "stable" and "Channel: stable" not in info:
            errors.append("BUILD-INFO does not identify stable channel")
    if package and package.is_file():
        text = archive_text(package)
        if channel == "stable":
            forbidden = ("1.1.0-rc1", "RELEASE-CANDIDATE", "NOT_TESTED", "NOT TESTED", "SKIPPED",
                         "NOT READY FOR 1.1.0 STABLE", "release_ready=false")
            for marker in forbidden:
                if marker in text:
                    errors.append(f"stable package contains stale/conflicting marker: {marker}")
            with zipfile.ZipFile(package) as archive:
                if any(name.endswith("/RELEASE-CANDIDATE.md") for name in archive.namelist()):
                    errors.append("stable package contains RELEASE-CANDIDATE.md")
    return not errors, errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validation-json", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--validation-text", type=Path)
    parser.add_argument("--build-info", type=Path)
    parser.add_argument("--package", type=Path)
    parser.add_argument("--channel", choices=("rc", "stable"), required=True)
    args = parser.parse_args()
    try:
        validation = json.loads(args.validation_json.read_text(encoding="utf-8"))
        evidence_ok, evidence_rows = audit_evidence(validation, args.validation_json.parent)
        docs_ok, doc_errors = audit_docs(validation, args.validation_text, args.build_info, args.package, args.channel)
        result = {
            "test": "release_validation_audit",
            "evidence_integrity": evidence_ok,
            "documentation_consistency": docs_ok,
            "passed": evidence_ok and docs_ok,
            "evidence": evidence_rows,
            "documentation_errors": doc_errors,
        }
        write(args.output, result)
        return 0 if result["passed"] else 1
    except (OSError, ValueError, json.JSONDecodeError, zipfile.BadZipFile) as exc:
        write(args.output, {"test": "release_validation_audit", "passed": False, "reason": str(exc)})
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
