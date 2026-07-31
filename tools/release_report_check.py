#!/usr/bin/env python3
"""Fail-closed validation for machine-readable OmniPack release reports."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import sys
from typing import Mapping, Sequence


REQUIRED_FIELDS = (
    "source_commit",
    "release_tag",
    "version",
    "upstream_version",
    "clean_build_pass",
    "meson_tests",
    "python_tests",
    "lua_runtime_tests",
    "zh_gui_test",
    "en_gui_test",
    "font_visual_test",
    "module_ui_test",
    "ops_roundtrip_test",
    "disabled_module_dialog_test",
    "readonly_save_block_test",
    "readonly_upload_block_test",
    "save_migration_test",
    "reaction_tests",
    "automation_tests",
    "game_tasks_removed",
    "achievements_removed",
    "challenge_system_removed",
    "technology_tree_removed",
    "alchemy_progression_removed",
    "forced_unlocks_removed",
    "stress_test",
    "long_run_test",
    "font_license_resolved",
    "third_party_license_audit",
    "secret_scan_pass",
    "source_commit_public",
    "anonymous_clone_pass",
    "release_exe_stripped",
    "debug_symbols_separated",
    "developer_paths_removed",
    "pe_security_flags_preserved",
    "authenticode_signed",
    "public_zip_sha256",
    "symbols_zip_sha256",
    "source_zip_sha256",
    "zip_audit_pass",
    "github_release_created",
    "release_ready",
)

BOOLEAN_OR_NOT_TESTED = re.compile(r"^(?:true|false|not_tested)$")
NUMBER = re.compile(r"^\d+(?:\.\d+)?$")
RATIO = re.compile(r"^\d+/\d+$")
COMMIT = re.compile(r"^[0-9a-f]{40}$")
SHA256 = re.compile(r"^[0-9A-F]{64}$")
VERSION = re.compile(r"^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$")
TAG = re.compile(r"^(?:not_tested|v\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?)$")

HASH_ARTIFACTS = {
    "public_zip_sha256": "TPT-ZH-OmniPack-{version}-Windows-x64.zip",
    "symbols_zip_sha256": "TPT-ZH-OmniPack-{version}-Symbols-Windows-x64.zip",
    "source_zip_sha256": "TPT-ZH-OmniPack-{version}-Source.zip",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def parse_report(text: str) -> dict[str, str]:
    heading = "## 机器可读结论"
    heading_index = text.find(heading)
    if heading_index < 0:
        raise ValueError(f"missing heading: {heading}")
    fence_start = text.find("```text", heading_index)
    if fence_start < 0:
        raise ValueError("missing machine-readable text fence")
    content_start = text.find("\n", fence_start)
    fence_end = text.find("```", content_start + 1)
    if content_start < 0 or fence_end < 0:
        raise ValueError("unterminated machine-readable text fence")
    fields: dict[str, str] = {}
    for line_number, raw in enumerate(
        text[content_start + 1 : fence_end].splitlines(), start=1
    ):
        line = raw.strip()
        if not line:
            continue
        if "=" not in line:
            raise ValueError(f"machine line {line_number} is not key=value")
        key, value = line.split("=", 1)
        if not re.fullmatch(r"[a-z][a-z0-9_]*", key):
            raise ValueError(f"invalid machine key: {key!r}")
        if key in fields:
            raise ValueError(f"duplicate machine key: {key}")
        if not value or value != value.strip():
            raise ValueError(f"invalid machine value for {key}")
        fields[key] = value
    return fields


def validate_values(fields: Mapping[str, str]) -> list[str]:
    errors: list[str] = []
    missing = [field for field in REQUIRED_FIELDS if field not in fields]
    if missing:
        errors.append("missing required fields: " + ", ".join(missing))

    semantic_fields = {
        "source_commit",
        "release_tag",
        "version",
        "upstream_version",
        "public_zip_sha256",
        "symbols_zip_sha256",
        "source_zip_sha256",
    }
    for key, value in fields.items():
        if key in semantic_fields:
            continue
        if not (
            BOOLEAN_OR_NOT_TESTED.fullmatch(value)
            or NUMBER.fullmatch(value)
            or RATIO.fullmatch(value)
            or COMMIT.fullmatch(value)
            or SHA256.fullmatch(value)
        ):
            errors.append(f"{key} has a non-machine value: {value!r}")

    source_commit = fields.get("source_commit", "")
    if not COMMIT.fullmatch(source_commit):
        errors.append("source_commit must be a lowercase 40-character commit")
    if not TAG.fullmatch(fields.get("release_tag", "")):
        errors.append("release_tag is invalid")
    for key in ("version", "upstream_version"):
        if not VERSION.fullmatch(fields.get(key, "")):
            errors.append(f"{key} is invalid")
    for key in HASH_ARTIFACTS:
        value = fields.get(key, "")
        if value != "not_tested" and not SHA256.fullmatch(value):
            errors.append(f"{key} must be uppercase SHA-256 or not_tested")

    if fields.get("release_ready") == "true":
        required_true = (
            "clean_build_pass",
            "zh_gui_test",
            "en_gui_test",
            "font_visual_test",
            "module_ui_test",
            "ops_roundtrip_test",
            "disabled_module_dialog_test",
            "readonly_save_block_test",
            "readonly_upload_block_test",
            "game_tasks_removed",
            "achievements_removed",
            "challenge_system_removed",
            "technology_tree_removed",
            "alchemy_progression_removed",
            "forced_unlocks_removed",
            "stress_test",
            "long_run_test",
            "font_license_resolved",
            "third_party_license_audit",
            "secret_scan_pass",
            "source_commit_public",
            "anonymous_clone_pass",
            "release_exe_stripped",
            "debug_symbols_separated",
            "developer_paths_removed",
            "pe_security_flags_preserved",
            "zip_audit_pass",
            "github_release_created",
        )
        inconsistent = [key for key in required_true if fields.get(key) != "true"]
        if inconsistent:
            errors.append(
                "release_ready=true conflicts with: " + ", ".join(inconsistent)
            )
        expected_tag = "v" + fields.get("version", "")
        if fields.get("release_tag") != expected_tag:
            errors.append(f"release_ready=true requires release_tag={expected_tag}")
    return errors


def verify_artifacts(fields: Mapping[str, str], dist: Path) -> list[str]:
    errors: list[str] = []
    version = fields.get("version", "")
    for field, template in HASH_ARTIFACTS.items():
        expected = fields.get(field, "")
        if expected == "not_tested":
            continue
        path = dist / template.format(version=version)
        if not path.is_file():
            errors.append(f"{field} artifact is missing: {path.name}")
            continue
        actual = sha256(path)
        if actual != expected:
            errors.append(f"{field} mismatch: report={expected}, actual={actual}")
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "report",
        nargs="?",
        type=Path,
        default=Path("dist/release-report-0.1.0-test.md"),
    )
    parser.add_argument("--verify-artifacts", action="store_true")
    parser.add_argument("--dist", type=Path)
    parser.add_argument("--quiet", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    report = args.report.resolve()
    try:
        fields = parse_report(report.read_text(encoding="utf-8"))
        errors = validate_values(fields)
        if args.verify_artifacts:
            dist = args.dist.resolve() if args.dist else report.parent
            errors.extend(verify_artifacts(fields, dist))
    except (OSError, ValueError) as exc:
        errors = [str(exc)]
        fields = {}
    if errors:
        for error in errors:
            print(f"release-report-check: ERROR {error}", file=sys.stderr)
        return 1
    if not args.quiet:
        print(
            "release-report-check: PASS "
            f"fields={len(fields)} release_ready={fields['release_ready']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
