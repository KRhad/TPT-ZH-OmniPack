#!/usr/bin/env python3
"""Fail closed when a test or release-candidate ZIP is incomplete or unsafe."""

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
AUTOMATION_VERSION = "0.3.0-dev"
PREVIOUS_PRIVATE_TEST_VERSION = "0.6.0-dev"
PRIVATE_TEST_VERSION = "0.7.0-dev"
RELEASE_CANDIDATE_VERSION = "1.0.0-rc5"
PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Windows-x64"
SYMBOL_PACKAGE_STEM = f"TPT-ZH-OmniPack-{VERSION}-Symbols-Windows-x64"
EXECUTABLE_NAME = "tpt-zh-omnipack.exe"
SYMBOL_NAME = "tpt-zh-omnipack.debug"
NORMAL_DOCUMENTS = {
    "LICENSE", "README.en.md", "README.zh-CN.md", "CHANGELOG.en.txt",
    "CHANGELOG.zh-CN.md", "TESTING.zh-CN.md",
    "SOURCE-AND-LICENSES.zh-CN.md", "KNOWN-ISSUES.zh-CN.md", "AI-DISCLOSURE.zh-CN.md",
    "FONT-AUDIT.md", "LICENSES/GNU-UNIFONT-OFL-1.1.txt",
    "LICENSES/THIRD-PARTY-MANIFEST.csv",
    "LICENSES/FUSION-PIXEL-FONT-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-ARK-PIXEL-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-CUBIC-11-OFL-1.1.txt",
    "LICENSES/FUSION-PIXEL-FONT-GALMURI-OFL-1.1.txt",
    *{
        f"LICENSES/LIBRARIES/{name}.LICENSE.txt"
        for name in (
            "bzip2", "fftw3f", "jsoncpp", "libcurl", "libpng", "lua5.1",
            "lua5.2", "luajit", "mbedtls", "nghttp2", "sdl2", "zlib",
        )
    },
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
AUTOMATION_ONLY_DOCUMENTS = {
    "AUTOMATION-CAPABILITIES.csv",
    "automation/0.3.0/scenario-spec.json",
    "automation/0.3.0/official-source-sha256.json",
    "examples/0.3.0/manifest.json",
    "examples/0.3.0/runtime-report.json",
    "examples/0.3.0/01-thermostatic-furnace.stm",
    "examples/0.3.0/02-automatic-alloy.stm",
    "examples/0.3.0/03-fuel-control.stm",
    "examples/0.3.0/04-nutrient-dosing.stm",
    "examples/0.3.0/05-pathogen-disinfection.stm",
    "examples/0.3.0/06-reactor-cooling.stm",
    "examples/0.3.0/07-emergency-stop.stm",
    "examples/0.3.0/08-waste-transfer.stm",
    "examples/0.3.0/09-integrated-factory.stm",
}
AUTOMATION_DOCUMENTS = DEV_DOCUMENTS | AUTOMATION_ONLY_DOCUMENTS
PRIVATE_TEST_MARKERS = {
    PREVIOUS_PRIVATE_TEST_VERSION: (
        PREVIOUS_PRIVATE_TEST_VERSION,
        "不是 1.0.0 正式版",
        "451",
        "118/118",
        "621",
        "release_ready=false",
    ),
    PRIVATE_TEST_VERSION: (
        PRIVATE_TEST_VERSION,
        "不是 1.0.0 正式版",
        "487",
        "466",
        "118/118",
        "685",
        "release_ready=false",
    ),
}
RELEASE_CANDIDATE_MARKERS = {
    RELEASE_CANDIDATE_VERSION: (
        RELEASE_CANDIDATE_VERSION,
        "本地发布候选",
        "不是正式发布",
        "487",
        "484",
        "466",
        "118/118",
        "release_ready=false",
        "未签名",
    ),
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


def development_documents(version: str) -> set[str]:
    if version == DEV_VERSION:
        return DEV_DOCUMENTS
    if version == AUTOMATION_VERSION:
        return AUTOMATION_DOCUMENTS
    return set()


def expected_members(stem: str, kind: str, version: str) -> set[str]:
    if kind in {"public-test", "release-candidate"}:
        files = {EXECUTABLE_NAME, *NORMAL_DOCUMENTS}
    elif kind == "local-dev":
        files = {
            EXECUTABLE_NAME,
            *NORMAL_DOCUMENTS,
            *development_documents(version),
        }
    elif kind == "debug-symbols":
        files = {SYMBOL_NAME}
    else:
        return set()
    return {f"{stem}/{name}" for name in files | {"TEST-MANIFEST.txt"}}


def audit_0_2_examples(
    archive: zipfile.ZipFile,
    stem: str,
    actual: set[str],
    executable_hash: str,
    errors: list[str],
) -> None:
    example_manifest = json.loads(
        archive.read(f"{stem}/examples/0.2.0/manifest.json")
    )
    runtime_report = json.loads(
        archive.read(f"{stem}/examples/0.2.0/tutorials-runtime-report.json")
    )
    if example_manifest.get("content_version") != DEV_VERSION:
        errors.append("0.2 example manifest version is invalid")
    if example_manifest.get("generator_exe_sha256") != executable_hash:
        errors.append("0.2 example manifest executable does not match package")
    if runtime_report.get("executable_sha256") != executable_hash:
        errors.append("0.2 tutorial report executable does not match package")
    if runtime_report.get("source_commit") != example_manifest.get("source_commit"):
        errors.append("0.2 tutorial and example source commits do not match")
    if runtime_report.get("pass_count") != 8:
        errors.append("0.2 tutorial report does not contain 8 passes")
    example_rows = example_manifest.get("examples")
    if not isinstance(example_rows, list) or len(example_rows) != 7:
        errors.append("0.2 example manifest does not contain 7 examples")
        return
    for row in example_rows:
        filename = row.get("filename") if isinstance(row, dict) else None
        member_name = f"{stem}/examples/0.2.0/{filename}"
        if member_name not in actual:
            errors.append(f"0.2 example manifest member is missing: {filename}")
            continue
        data = archive.read(member_name)
        if len(data) != row.get("bytes") or sha256_bytes(data) != row.get("sha256"):
            errors.append(f"0.2 example manifest does not match member: {filename}")


def audit_0_3_automation(
    archive: zipfile.ZipFile,
    stem: str,
    actual: set[str],
    executable_hash: str,
    errors: list[str],
) -> None:
    manifest = json.loads(archive.read(f"{stem}/examples/0.3.0/manifest.json"))
    report = json.loads(archive.read(f"{stem}/examples/0.3.0/runtime-report.json"))
    if manifest.get("content_version") != AUTOMATION_VERSION:
        errors.append("0.3 automation manifest version is invalid")
    if manifest.get("source_tree_state") != "clean":
        errors.append("0.3 automation manifest is not from a clean source tree")
    if manifest.get("generator_exe_sha256") != executable_hash:
        errors.append("0.3 automation manifest executable does not match package")
    if report.get("executable_sha256") != executable_hash:
        errors.append("0.3 automation report executable does not match package")
    if report.get("source_commit") != manifest.get("source_commit"):
        errors.append("0.3 automation report and manifest source commits do not match")
    if report.get("source_tree_state") != "clean":
        errors.append("0.3 automation report is not from a clean source tree")
    if report.get("scenario_pass_count") != 9:
        errors.append("0.3 automation report does not contain 9 scenario passes")
    if report.get("challenge_pass_count") != 6:
        errors.append("0.3 automation report does not contain 6 challenge passes")
    if report.get("stop_event_delta_total") != 0:
        errors.append("0.3 automation report contains post-stop events")
    challenge_ids = manifest.get("challenge_ids")
    if not isinstance(challenge_ids, list) or len(challenge_ids) != 6:
        errors.append("0.3 automation manifest does not contain 6 challenge IDs")
    scenario_rows = manifest.get("scenarios")
    if not isinstance(scenario_rows, list) or len(scenario_rows) != 9:
        errors.append("0.3 automation manifest does not contain 9 scenarios")
        return
    stable_ids: set[str] = set()
    for row in scenario_rows:
        filename = row.get("filename") if isinstance(row, dict) else None
        stable_id = row.get("stamp_id") if isinstance(row, dict) else None
        if not isinstance(stable_id, str) or not re.fullmatch(r"03[0-9a-f]{8}", stable_id):
            errors.append(f"0.3 automation scenario has invalid stable stamp ID: {stable_id!r}")
        elif stable_id in stable_ids:
            errors.append(f"0.3 automation scenario repeats stable stamp ID: {stable_id}")
        else:
            stable_ids.add(stable_id)
        member_name = f"{stem}/examples/0.3.0/{filename}"
        if member_name not in actual:
            errors.append(f"0.3 automation manifest member is missing: {filename}")
            continue
        data = archive.read(member_name)
        if len(data) != row.get("bytes") or sha256_bytes(data) != row.get("sha256"):
            errors.append(f"0.3 automation manifest does not match member: {filename}")


def audit_package(
    package_path: Path,
    expect_symbols: bool = False,
    version: str = VERSION,
    kind: str | None = None,
) -> list[str]:
    errors: list[str] = []
    profiles = {
        VERSION: "public-test",
        DEV_VERSION: "local-dev",
        AUTOMATION_VERSION: "local-dev",
        **{
            private_version: "local-dev"
            for private_version in PRIVATE_TEST_MARKERS
        },
        RELEASE_CANDIDATE_VERSION: "release-candidate",
    }
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
            expected = expected_members(stem, kind, version)
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
                allowed_stamp = (
                    kind == "local-dev"
                    and relative in development_documents(version)
                )
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
                    executable_hash = sha256_bytes(executable)
                    if version in {DEV_VERSION, AUTOMATION_VERSION}:
                        audit_0_2_examples(
                            archive, stem, actual, executable_hash, errors
                        )
                    if version == AUTOMATION_VERSION:
                        audit_0_3_automation(
                            archive, stem, actual, executable_hash, errors
                        )
                    if version in PRIVATE_TEST_MARKERS:
                        instructions = archive.read(
                            f"{stem}/TESTING.zh-CN.md"
                        ).decode("utf-8", errors="replace")
                        for marker in PRIVATE_TEST_MARKERS[version]:
                            if marker not in instructions:
                                errors.append(
                                    f"private test instructions are missing marker: {marker!r}"
                                )
                elif kind == "release-candidate":
                    instructions = archive.read(
                        f"{stem}/TESTING.zh-CN.md"
                    ).decode("utf-8", errors="replace")
                    for marker in RELEASE_CANDIDATE_MARKERS[version]:
                        if marker not in instructions:
                            errors.append(
                                f"release candidate instructions are missing marker: {marker!r}"
                            )
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
    parser.add_argument(
        "--version",
        choices=(
            VERSION,
            DEV_VERSION,
            AUTOMATION_VERSION,
            *PRIVATE_TEST_MARKERS,
            *RELEASE_CANDIDATE_MARKERS,
        ),
        default=VERSION,
    )
    parser.add_argument(
        "--kind", choices=("public-test", "local-dev", "release-candidate")
    )
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
