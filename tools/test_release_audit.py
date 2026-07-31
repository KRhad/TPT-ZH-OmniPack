#!/usr/bin/env python3
"""Fail closed when a public test-release ZIP is incomplete or unsafe."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
from typing import Sequence
import zipfile


VERSION = "0.1.0-test"
DEV_VERSION = "0.2.0-dev"
PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Windows-x64"
SYMBOL_PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Symbols-Windows-x64"
EXECUTABLE_NAME = "tpt-zh-omnipack.exe"
SYMBOL_NAME = "tpt-zh-omnipack.debug"
NORMAL_DOCUMENTS = {
    "LICENSE", "README.zh-CN.md", "CHANGELOG.zh-CN.md", "TESTING.zh-CN.md",
    "SOURCE-AND-LICENSES.zh-CN.md", "KNOWN-ISSUES.zh-CN.md", "AI-DISCLOSURE.zh-CN.md",
    "FONT-AUDIT.md", "LICENSES/GNU-UNIFONT-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-ARK-PIXEL-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-CUBIC-11-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-GALMURI-OFL-1.1.txt",
}
DEV_DOCUMENTS = {
    "TUTORIALS-0.2.0.json",
    "examples/0.2.0/example-spec.json",
    "examples/0.2.0/manifest.json",
    "examples/0.2.0/tutorials-runtime-report.json",
    "examples/0.2.0/01-peroxide-pathogen.stm",
    "examples/0.2.0/02-humus-fertilizer.stm",
    "examples/0.2.0/03-slag-acid.stm",
    "examples/0.2.0/04-shield-assembly.stm",
    "examples/0.2.0/05-waste-stabilization.stm",
    "examples/0.2.0/06-waste-missing-catalyst.stm",
    "examples/0.2.0/07-integrated-recovery.stm",
}
FORBIDDEN_SUFFIXES = (".cps", ".stm", ".pref", ".lua", ".o", ".obj", ".pdb", ".dmp")
PATH_MARKERS = (b"C:\\Users\\", b"/Users/", b"\\build-", b"/build-")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def parse_manifest(data: bytes) -> tuple[dict[str, str], dict[str, tuple[int, str]]]:
    fields: dict[str, str] = {}
    members: dict[str, tuple[int, str]] = {}
    for line in data.decode("utf-8").splitlines():
        if line.startswith("member="):
            name, size, digest = line.removeprefix("member=").split("|", 2)
            if name in members:
                raise ValueError(f"duplicate manifest member: {name}")
            members[name] = (int(size), digest)
        elif "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value
    return fields, members


def package_stems(version: str) -> tuple[str, str]:
    return (
        f"TPT-ZH-OmniPack-{version}-Windows-x64",
        f"TPT-ZH-OmniPack-{version}-Symbols-Windows-x64",
    )


def expected_members(stem: str, kind: str) -> set[str]:
    if kind == "public-test":
        files = {EXECUTABLE_NAME, *NORMAL_DOCUMENTS}
    elif kind == "local-dev":
        files = {EXECUTABLE_NAME, *NORMAL_DOCUMENTS, *DEV_DOCUMENTS}
    elif kind == "debug-symbols":
        files = {SYMBOL_NAME}
    else:
        return set()
    return {f"{stem}/{name}" for name in files | {"TEST-MANIFEST.txt"}}


def audit_package(
    package_path: Path,
    expect_symbols: bool = False,
    version: str = VERSION,
    kind: str | None = None,
) -> list[str]:
    errors: list[str] = []
    profiles = {VERSION: "public-test", DEV_VERSION: "local-dev"}
    if version not in profiles:
        return [f"unsupported package version: {version}"]
    if expect_symbols:
        kind = "debug-symbols"
    elif kind is None:
        kind = profiles[version]
    elif kind != profiles[version]:
        return [f"version {version} requires kind={profiles[version]}"]
    package_stem, symbol_package_stem = package_stems(version)
    stem = symbol_package_stem if expect_symbols else package_stem
    if not package_path.is_file():
        return [f"package does not exist: {package_path}"]
    try:
        with zipfile.ZipFile(package_path) as archive:
            names = archive.namelist()
            if len(set(names)) != len(names):
                errors.append("package contains duplicate ZIP member names")
            actual = set(names)
            expected = expected_members(stem, kind)
            if actual != expected:
                missing = sorted(expected - actual)
                extra = sorted(actual - expected)
                if missing:
                    errors.append(f"package is missing members: {', '.join(missing)}")
                if extra:
                    errors.append(f"package contains unexpected members: {', '.join(extra)}")
            for name in actual:
                lowered = name.lower()
                relative = name.removeprefix(f"{stem}/")
                allowed_stamp = kind == "local-dev" and relative in DEV_DOCUMENTS
                if lowered.endswith(FORBIDDEN_SUFFIXES) and not allowed_stamp:
                    errors.append(f"package contains forbidden data: {name}")
            manifest_name = f"{stem}/TEST-MANIFEST.txt"
            if manifest_name not in actual:
                errors.append("package manifest is missing")
                return errors
            fields, members = parse_manifest(archive.read(manifest_name))
            if fields.get("format") != "2" or fields.get("kind") != kind or fields.get("version") != version:
                errors.append("package manifest metadata is invalid")
            if not re.fullmatch(r"[0-9a-f]{40}", fields.get("revision", "")):
                errors.append("package manifest Git revision is invalid")
            expected_relative = {name.removeprefix(f"{stem}/") for name in expected - {manifest_name}}
            if set(members) != expected_relative:
                errors.append("package manifest members do not match ZIP members")
            for name, (size, digest) in members.items():
                archive_name = f"{stem}/{name}"
                if archive_name not in actual:
                    continue
                data = archive.read(archive_name)
                if len(data) != size or sha256_bytes(data) != digest:
                    errors.append(f"package manifest does not match member: {name}")
            if not expect_symbols:
                executable = archive.read(f"{stem}/{EXECUTABLE_NAME}")
                if not executable.startswith(b"MZ"):
                    errors.append("packaged executable does not begin with PE MZ header")
                if any(marker.lower() in executable.lower() for marker in PATH_MARKERS):
                    errors.append("packaged executable leaks a development path")
                if kind == "public-test":
                    for marker in ("Windows x64", "不属于本测试版", "已知测试限制", "未签名"):
                        if marker not in archive.read(f"{stem}/TESTING.zh-CN.md").decode("utf-8", errors="replace"):
                            errors.append(f"test instructions are missing marker: {marker!r}")
                elif kind == "local-dev":
                    example_manifest = json.loads(
                        archive.read(f"{stem}/examples/0.2.0/manifest.json")
                    )
                    runtime_report = json.loads(
                        archive.read(
                            f"{stem}/examples/0.2.0/tutorials-runtime-report.json"
                        )
                    )
                    executable_hash = sha256_bytes(executable)
                    if example_manifest.get("content_version") != version:
                        errors.append("example manifest version does not match package")
                    if example_manifest.get("generator_exe_sha256") != executable_hash:
                        errors.append("example manifest executable does not match package")
                    if runtime_report.get("executable_sha256") != executable_hash:
                        errors.append("tutorial report executable does not match package")
                    if runtime_report.get("source_commit") != example_manifest.get("source_commit"):
                        errors.append("tutorial and example source commits do not match")
                    if runtime_report.get("pass_count") != 8:
                        errors.append("tutorial report does not contain 8 passes")
                    example_rows = example_manifest.get("examples")
                    if not isinstance(example_rows, list) or len(example_rows) != 7:
                        errors.append("example manifest does not contain 7 examples")
                    else:
                        for row in example_rows:
                            filename = row.get("filename") if isinstance(row, dict) else None
                            member_name = f"{stem}/examples/0.2.0/{filename}"
                            if member_name not in actual:
                                errors.append(f"example manifest member is missing: {filename}")
                                continue
                            data = archive.read(member_name)
                            if len(data) != row.get("bytes") or sha256_bytes(data) != row.get("sha256"):
                                errors.append(f"example manifest does not match member: {filename}")
    except (OSError, ValueError, json.JSONDecodeError, zipfile.BadZipFile) as exc:
        errors.append(f"cannot read package: {exc}")
    return errors


def audit_hash_file(package_path: Path) -> list[str]:
    hash_path = package_path.with_suffix(package_path.suffix + ".sha256")
    if not hash_path.is_file():
        return [f"ZIP hash file is missing: {hash_path}"]
    expected = f"{sha256_bytes(package_path.read_bytes())}  {package_path.name}\n"
    return [] if hash_path.read_text(encoding="ascii") == expected else ["ZIP hash file does not match archive"]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("--symbols", action="store_true")
    parser.add_argument("--version", choices=(VERSION, DEV_VERSION), default=VERSION)
    parser.add_argument("--kind", choices=("public-test", "local-dev"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    errors = audit_package(
        args.package,
        args.symbols,
        version=args.version,
        kind=args.kind,
    ) + audit_hash_file(args.package)
    if errors:
        for error in errors:
            print(f"test-release-audit: ERROR {error}", file=sys.stderr)
        return 1
    print("test-release-audit: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
