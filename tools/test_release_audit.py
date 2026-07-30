#!/usr/bin/env python3
"""Fail closed when a generated test release contains unexpected or unsafe files."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys
from typing import Sequence
import zipfile


PACKAGE_NAME = "TPT-ZH-OmniPack-Test-Windows-x64"
EXPECTED_MEMBERS = {
    f"{PACKAGE_NAME}/CHANGELOG.zh-CN.md",
    f"{PACKAGE_NAME}/LICENSE",
    f"{PACKAGE_NAME}/README.zh-CN.md",
    f"{PACKAGE_NAME}/TEST-MANIFEST.txt",
    f"{PACKAGE_NAME}/TESTING.zh-CN.md",
    f"{PACKAGE_NAME}/tpt-zh-omnipack.exe",
}
FORBIDDEN_SUFFIXES = (".cps", ".stm", ".pref", ".lua")
REQUIRED_TESTING_MARKERS = (
    "Windows x64",
    "不属于本测试版",
    "已知测试限制",
)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def parse_manifest(data: bytes) -> dict[str, str]:
    manifest: dict[str, str] = {}
    for line in data.decode("utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            manifest[key] = value
    return manifest


def audit_package(package_path: Path) -> list[str]:
    errors: list[str] = []
    if not package_path.is_file():
        return [f"package does not exist: {package_path}"]
    try:
        with zipfile.ZipFile(package_path) as archive:
            raw_names = archive.namelist()
            names = set(raw_names)
            if len(names) != len(raw_names):
                errors.append("package contains duplicate ZIP member names")
            if names != EXPECTED_MEMBERS:
                missing = sorted(EXPECTED_MEMBERS - names)
                extra = sorted(names - EXPECTED_MEMBERS)
                if missing:
                    errors.append(f"package is missing members: {', '.join(missing)}")
                if extra:
                    errors.append(f"package contains unexpected members: {', '.join(extra)}")
            for name in names:
                lowered = name.lower()
                if lowered.endswith(FORBIDDEN_SUFFIXES):
                    errors.append(f"package contains forbidden personal or script data: {name}")
            executable_name = f"{PACKAGE_NAME}/tpt-zh-omnipack.exe"
            manifest_name = f"{PACKAGE_NAME}/TEST-MANIFEST.txt"
            testing_name = f"{PACKAGE_NAME}/TESTING.zh-CN.md"
            if executable_name in names:
                executable = archive.read(executable_name)
                if not executable.startswith(b"MZ"):
                    errors.append("packaged executable does not begin with PE MZ header")
            else:
                executable = b""
            if manifest_name in names:
                manifest = parse_manifest(archive.read(manifest_name))
                if manifest.get("format") != "1":
                    errors.append("package manifest format is missing or unsupported")
                if manifest.get("file") != "tpt-zh-omnipack.exe":
                    errors.append("package manifest executable name is incorrect")
                if executable and manifest.get("sha256") != sha256_bytes(executable):
                    errors.append("package manifest executable SHA-256 does not match")
                if executable and manifest.get("size") != str(len(executable)):
                    errors.append("package manifest executable size does not match")
            else:
                errors.append("package manifest is missing")
            if testing_name in names:
                testing_text = archive.read(testing_name).decode("utf-8", errors="replace")
                for marker in REQUIRED_TESTING_MARKERS:
                    if marker not in testing_text:
                        errors.append(f"test instructions are missing marker: {marker!r}")
            else:
                errors.append("test instructions are missing")
    except (OSError, zipfile.BadZipFile) as exc:
        errors.append(f"cannot read package: {exc}")
    return errors


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit_package(args.package)
    if errors:
        for error in errors:
            print(f"test-release-audit: ERROR {error}", file=sys.stderr)
        return 1
    print("test-release-audit: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
